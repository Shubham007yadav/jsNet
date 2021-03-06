
ConvLayer::ConvLayer (int netI, int s) : Layer(netI, s) {
    netInstance = netI;
    size = s;
    type = "Conv";
    hasActivation = false;
}

ConvLayer::~ConvLayer (void) {
    for (int f=0; f<filters.size(); f++) {
        delete filters[f];
    }
}

void ConvLayer::assignNext (Layer* l) {
    nextLayer = l;
}

void ConvLayer::assignPrev (Layer* l) {
    prevLayer = l;

    for (int f=0; f<size; f++) {
        filters.push_back(new Filter());
    }
}

void ConvLayer::init (int layerIndex) {

    Network* net = Network::getInstance(netInstance);

    biases = std::vector<double>(filters.size(), 1);
    deltaBiases = std::vector<double>(filters.size(), 0);

    for (int f=0; f<filters.size(); f++) {

        filterWeights.push_back({});

        // Weights
        for (int c=0; c<channels; c++) {

            filterWeights[f].push_back({});

            for (int r=0; r<filterSize; r++) {
                filterWeights[f][c].push_back(net->weightInitFn(netInstance, layerIndex, filterSize));
            }
        }

        if (net->dropout != 1) {
            filters[f]->dropoutMap = NetUtil::createVolume<bool>(1, outMapSize, outMapSize, 0)[0];
        }

        filters[f]->init(netInstance, channels, filterSize);

        filterDeltaWeights.push_back(NetUtil::createVolume<double>(channels, filterSize, filterSize, 0));
    }

    errors = NetUtil::createVolume<double>(filters.size(), outMapSize, outMapSize, 0);
    activations = NetUtil::createVolume<double>(filters.size(), outMapSize, outMapSize, 0);
}

void ConvLayer::forward (void) {

    Network* net = Network::getInstance(netInstance);

    std::vector<std::vector<std::vector<double> > > actvs;

    if (prevLayer->type=="FC") {
        actvs = NetUtil::arrayToVolume(prevLayer->actvns, channels);
    } else {
        actvs = prevLayer->activations;
    }

    for (int f=0; f<filters.size(); f++) {

        filters[f]->sumMap = NetUtil::convolve(actvs, zeroPadding, filterWeights[f], channels, stride, biases[f]);

        for (int sumY=0; sumY<filters[f]->sumMap.size(); sumY++) {
            for (int sumX=0; sumX<filters[f]->sumMap.size(); sumX++) {

                if (net->dropout != 1) {
                    filters[f]->dropoutMap[sumY][sumX] = (double) rand() / (RAND_MAX) > net->dropout;
                }

                if (net->dropout != 1 && net->isTraining && filters[f]->dropoutMap[sumY][sumX]) {
                    activations[f][sumY][sumX] = 0;

                } else if (hasActivation) {

                    activations[f][sumY][sumX] = activationC(filters[f]->sumMap[sumY][sumX], false, filters[f]) / net->dropout;

                } else {
                    activations[f][sumY][sumX] = filters[f]->sumMap[sumY][sumX];
                }
            }
        }
    }
}

void ConvLayer::backward (bool lastLayer) {

    if (nextLayer->type == "FC") {

        // For each filter, build the errorMap from the weighted neuron errors in the next FCLayer corresponding to each value in the activation map
        for (int f=0; f<filters.size(); f++) {

            for (int emY=0; emY<errors[f].size(); emY++) {
                for (int emX=0; emX<errors[f].size(); emX++) {

                    int weightI = f * outMapSize*outMapSize + emY * errors[f].size() + emX;

                    for (int n=0; n < nextLayer->neurons.size(); n++) {
                        errors[f][emY][emX] += nextLayer->errs[n] * nextLayer->weights[n][weightI];
                    }
                }
            }
        }

    } else if (nextLayer->type == "Conv") {

        for (int f=0; f<filters.size(); f++) {
            errors[f] = NetUtil::buildConvErrorMap(outMapSize + nextLayer->zeroPadding*2, nextLayer, f);
        }

    } else {

        for (int f=0; f<filters.size(); f++) {
            for (int r=0; r<errors[f].size(); r++) {
                for (int v=0; v<errors[f].size(); v++) {
                    errors[f][r][v] = nextLayer->errors[f][r][v];
                }
            }
        }
    }

    // Apply derivative to each error value
    for (int f=0; f<filters.size(); f++) {
        for (int row=0; row<errors[f].size(); row++) {
            for (int col=0; col<errors[f][0].size(); col++) {

                if (filters[f]->dropoutMap.size() && filters[f]->dropoutMap[row][col]) {
                    errors[f][row][col] = 0;
                } else if (hasActivation) {
                    errors[f][row][col] *= activationC(filters[f]->sumMap[row][col], true, filters[f]);
                }
            }
        }
    }

    NetUtil::buildConvDWeights(this);
}

void ConvLayer::resetDeltaWeights (void) {

    deltaBiases = std::vector<double>(size, 0);

    for (int f=0; f<filters.size(); f++) {

        for (int c=0; c<filterDeltaWeights[f].size(); c++) {
            for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {
                    filterDeltaWeights[f][c][r][v] = 0;
                }
            }
        }

        for (int row=0; row<errors[f].size(); row++) {
            for (int col=0; col<errors[f].size(); col++) {
                errors[f][row][col] = 0;
            }
        }

        if (filters[f]->dropoutMap.size()) {
            for (int r=0; r<filters[f]->dropoutMap.size(); r++) {
                for (int c=0; c<filters[f]->dropoutMap[0].size(); c++) {
                    filters[f]->dropoutMap[r][c] = false;
                }
            }
        }
    }
}

void ConvLayer::applyDeltaWeights (void) {

    Network* net = Network::getInstance(netInstance);

    for (int f=0; f<filters.size(); f++) {
        for (int c=0; c<filterDeltaWeights[f].size(); c++) {
            for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {
                    if (net->l2) net->l2Error += 0.5 * net->l2 * pow(filterWeights[f][c][r][v], 2);
                    if (net->l1) net->l1Error += net->l1 * fabs(filterWeights[f][c][r][v]);
                }
            }
        }
    }

    // Function pointers are far too slow, for this
    // Using code repetitive switch statements makes a substantial perf difference
    // Doesn't mean I like it :(
    switch (net->updateFnIndex) {
        case 0: // vanilla
            for (int f=0; f<filters.size(); f++) {
                for (int c=0; c<filterDeltaWeights[f].size(); c++) {
                    for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                        for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {

                            double regularized = (filterDeltaWeights[f][c][r][v]
                                + net->l2 * filterWeights[f][c][r][v]
                                + net->l1 * (filterWeights[f][c][r][v] > 0 ? 1 : -1)) / net->miniBatchSize;

                            filterWeights[f][c][r][v] = NetMath::vanillasgd(netInstance, filterWeights[f][c][r][v], regularized);

                            if (net->maxNorm) net->maxNormTotal += filterWeights[f][c][r][v] * filterWeights[f][c][r][v];
                        }
                    }
                }
                biases[f] = NetMath::vanillasgd(netInstance, biases[f], deltaBiases[f]);
            }
            break;
        case 1: // gain
            for (int f=0; f<filters.size(); f++) {
                for (int c=0; c<filterDeltaWeights[f].size(); c++) {
                    for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                        for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {

                            double regularized = (filterDeltaWeights[f][c][r][v]
                                                + net->l2 * filterWeights[f][c][r][v]
                                                + net->l1 * (filterWeights[f][c][r][v] > 0 ? 1 : -1)) / net->miniBatchSize;

                            filterWeights[f][c][r][v] = NetMath::gain(netInstance, filterWeights[f][c][r][v], regularized, filters[f], c, r, v);

                            if (net->maxNorm) net->maxNormTotal += filterWeights[f][c][r][v] * filterWeights[f][c][r][v];
                        }
                    }
                }
                biases[f] = NetMath::gain(netInstance, biases[f], deltaBiases[f], filters[f], -1, -1, -1);
            }
            break;
        case 2: // adagrad
            for (int f=0; f<filters.size(); f++) {
                for (int c=0; c<filterDeltaWeights[f].size(); c++) {
                    for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                        for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {

                            double regularized = (filterDeltaWeights[f][c][r][v]
                                                + net->l2 * filterWeights[f][c][r][v]
                                                + net->l1 * (filterWeights[f][c][r][v] > 0 ? 1 : -1)) / net->miniBatchSize;

                            filterWeights[f][c][r][v] = NetMath::adagrad(netInstance, filterWeights[f][c][r][v], regularized, filters[f], c, r, v);

                            if (net->maxNorm) net->maxNormTotal += filterWeights[f][c][r][v] * filterWeights[f][c][r][v];
                        }
                    }
                }
                biases[f] = NetMath::adagrad(netInstance, biases[f], deltaBiases[f], filters[f], -1, -1, -1);
            }
            break;
        case 3: // rmsprop
            for (int f=0; f<filters.size(); f++) {
                for (int c=0; c<filterDeltaWeights[f].size(); c++) {
                    for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                        for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {

                            double regularized = (filterDeltaWeights[f][c][r][v]
                                                + net->l2 * filterWeights[f][c][r][v]
                                                + net->l1 * (filterWeights[f][c][r][v] > 0 ? 1 : -1)) / net->miniBatchSize;

                            filterWeights[f][c][r][v] = NetMath::rmsprop(netInstance, filterWeights[f][c][r][v], regularized, filters[f], c, r, v);

                            if (net->maxNorm) net->maxNormTotal += filterWeights[f][c][r][v] * filterWeights[f][c][r][v];
                        }
                    }
                }
                biases[f] = NetMath::rmsprop(netInstance, biases[f], deltaBiases[f], filters[f], -1, -1, -1);
            }
            break;
        case 4: // adam
            for (int f=0; f<filters.size(); f++) {
                for (int c=0; c<filterDeltaWeights[f].size(); c++) {
                    for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                        for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {

                            double regularized = (filterDeltaWeights[f][c][r][v]
                                                + net->l2 * filterWeights[f][c][r][v]
                                                + net->l1 * (filterWeights[f][c][r][v] > 0 ? 1 : -1)) / net->miniBatchSize;

                            filterWeights[f][c][r][v] = NetMath::adam(netInstance, filterWeights[f][c][r][v], regularized, filters[f], c, r, v);

                            if (net->maxNorm) net->maxNormTotal += filterWeights[f][c][r][v] * filterWeights[f][c][r][v];
                        }
                    }
                }
                biases[f] = NetMath::adam(netInstance, biases[f], deltaBiases[f], filters[f], -1, -1, -1);
            }
            break;
        case 5: // adadelta
            for (int f=0; f<filters.size(); f++) {
                for (int c=0; c<filterDeltaWeights[f].size(); c++) {
                    for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                        for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {

                            double regularized = (filterDeltaWeights[f][c][r][v]
                                                + net->l2 * filterWeights[f][c][r][v]
                                                + net->l1 * (filterWeights[f][c][r][v] > 0 ? 1 : -1)) / net->miniBatchSize;

                            filterWeights[f][c][r][v] = NetMath::adadelta(netInstance, filterWeights[f][c][r][v], regularized, filters[f], c, r, v);

                            if (net->maxNorm) net->maxNormTotal += filterWeights[f][c][r][v] * filterWeights[f][c][r][v];
                        }
                    }
                }
                biases[f] = NetMath::adadelta(netInstance, biases[f], deltaBiases[f], filters[f], -1, -1, -1);
            }
            break;
        case 6: // momentum
            for (int f=0; f<filters.size(); f++) {
                for (int c=0; c<filterDeltaWeights[f].size(); c++) {
                    for (int r=0; r<filterDeltaWeights[f][0].size(); r++) {
                        for (int v=0; v<filterDeltaWeights[f][0][0].size(); v++) {

                            double regularized = (filterDeltaWeights[f][c][r][v]
                                                + net->l2 * filterWeights[f][c][r][v]
                                                + net->l1 * (filterWeights[f][c][r][v] > 0 ? 1 : -1)) / net->miniBatchSize;

                            filterWeights[f][c][r][v] = NetMath::momentum(netInstance, filterWeights[f][c][r][v], regularized, filters[f], c, r, v);

                            if (net->maxNorm) net->maxNormTotal += filterWeights[f][c][r][v] * filterWeights[f][c][r][v];
                        }
                    }
                }
                biases[f] = NetMath::momentum(netInstance, biases[f], deltaBiases[f], filters[f], -1, -1, -1);
            }
            break;
    }

    if (net->maxNorm) {
        net->maxNormTotal = sqrt(net->maxNormTotal);
        NetMath::maxNorm(netInstance);
    }
}

void ConvLayer::backUpValidation (void) {
    validationBiases = biases;
    validationFilterWeights = filterWeights;
}

void ConvLayer::restoreValidation (void) {
    biases = validationBiases;
    filterWeights = validationFilterWeights;
}

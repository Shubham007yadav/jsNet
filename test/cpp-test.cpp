#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../dev/cpp/Network.cpp"
#include <vector>
#include <algorithm>

bool doublesAreEqual(double a, double b) {
    return a==b || std::abs(a-b)<std::abs(std::min(a,b))*std::numeric_limits<double>::epsilon();
}

/* Network */
TEST_CASE("Network::newNetwork - Appends a new instance to the Network::instances vector, returning instance index") {
    REQUIRE( Network::netInstances.size() == 0 );
    Network::newNetwork();
    REQUIRE( Network::netInstances.size() == 1 );
    Network::newNetwork();
    REQUIRE( Network::netInstances.size() == 2 );
}
TEST_CASE("Network::newNetwork - Returns the index of the newly created instance") {
    Network::deleteNetwork();
    REQUIRE( Network::newNetwork()==0 );
    REQUIRE( Network::newNetwork()==1 );
    REQUIRE( Network::newNetwork()==2 );
}

TEST_CASE("Network::getInstance - Returns the correct Network instance") {
    Network::netInstances[0]->learningRate = (float) 1;
    Network::netInstances[1]->learningRate = (float) 2;
    Network::netInstances[2]->learningRate = (float) 3;
    REQUIRE( Network::getInstance(0)->learningRate == (float) 1 );
    REQUIRE( Network::getInstance(1)->learningRate == (float) 2 );
    REQUIRE( Network::getInstance(2)->learningRate == (float) 3 );
}

TEST_CASE("Network::deleteNetwork - Deletes a network instance when given an index") {
    REQUIRE( Network::netInstances.size() == 3 );
    REQUIRE( Network::netInstances[1] != 0 );
    Network::deleteNetwork(1);
    REQUIRE(Network::netInstances[1] == 0);
}

TEST_CASE("Network::deleteNetwork - Deletes all network instances when no index is given") {
    REQUIRE( Network::netInstances.size() == 3 );
    Network::deleteNetwork();
    REQUIRE( Network::netInstances.size() == 0 );
}


TEST_CASE("Network::joinLayers - Assigns the network activation function to each layer") {
    Network::deleteNetwork();
    Network::newNetwork();
    Network::getInstance(0)->layers.push_back(new Layer(0, 3));
    Network::getInstance(0)->layers.push_back(new Layer(0, 3));
    Network::getInstance(0)->layers.push_back(new Layer(0, 3));
    Network::getInstance(0)->activation = &NetMath::sigmoid;

    REQUIRE( Network::getInstance(0)->layers[0]->activation != &NetMath::sigmoid );
    REQUIRE( Network::getInstance(0)->layers[1]->activation != &NetMath::sigmoid );
    Network::getInstance(0)->joinLayers();
    REQUIRE( Network::getInstance(0)->layers[0]->activation == &NetMath::sigmoid );
    REQUIRE( Network::getInstance(0)->layers[1]->activation == &NetMath::sigmoid );
}

TEST_CASE("Network::joinLayers - Assigns prevLayers to layers accordingly") {
    REQUIRE( Network::getInstance(0)->layers[1]->prevLayer == Network::getInstance(0)->layers[0] );
    REQUIRE( Network::getInstance(0)->layers[2]->prevLayer == Network::getInstance(0)->layers[1] );
}

TEST_CASE("Network::joinLayers - Assigns nextLayers to layers accordingly") {
    REQUIRE( Network::getInstance(0)->layers[0]->nextLayer == Network::getInstance(0)->layers[1] );
    REQUIRE( Network::getInstance(0)->layers[1]->nextLayer == Network::getInstance(0)->layers[2] );
}


TEST_CASE("Network::forward - Sets the first layer's neurons' activations to the input given") {

    std::vector<double> testInput = {1,2,3};

    REQUIRE(Network::getInstance(0)->layers[0]->neurons[0]->activation != 1);
    REQUIRE(Network::getInstance(0)->layers[0]->neurons[1]->activation != 2);
    REQUIRE(Network::getInstance(0)->layers[0]->neurons[2]->activation != 3);

    Network::getInstance(0)->forward(testInput);

    REQUIRE(Network::getInstance(0)->layers[0]->neurons[0]->activation == 1);
    REQUIRE(Network::getInstance(0)->layers[0]->neurons[1]->activation == 2);
    REQUIRE(Network::getInstance(0)->layers[0]->neurons[2]->activation == 3);
}


TEST_CASE("Network::forward - Returns a vector of activations in the last layer") {

    std::vector<double> testInput = {1,2,3};
    std::vector<double> returned = Network::getInstance(0)->forward(testInput);

    std::vector<double> actualValues;

    for (int i=0; i<3; i++) {
        actualValues.push_back(Network::getInstance(0)->layers[2]->neurons[i]->activation);
    }

    REQUIRE( returned == actualValues );
}


TEST_CASE("Network::resetDeltaWeights - Sets all the delta weights values to 0") {
    Network::deleteNetwork();
    Network::newNetwork();
    Network::getInstance(0)->layers.push_back(new Layer(0, 3));
    Network::getInstance(0)->layers.push_back(new Layer(0, 3));
    Network::getInstance(0)->layers.push_back(new Layer(0, 3));
    Network::getInstance(0)->joinLayers();

    for (int n=1; n<3; n++) {
        Network::getInstance(0)->layers[0]->neurons[n]->deltaWeights = {1,1,1};
        Network::getInstance(0)->layers[1]->neurons[n]->deltaWeights = {1,1,1};
    }

    Network::getInstance(0)->resetDeltaWeights();
    std::vector<double> expected = {0,0,0};

    for (int n=1; n<3; n++) {
        REQUIRE( Network::getInstance(0)->layers[1]->neurons[n]->deltaWeights == expected );
        REQUIRE( Network::getInstance(0)->layers[2]->neurons[n]->deltaWeights == expected );
    }
}

TEST_CASE("Network::applyDeltaWeights - Increment the weights by the delta weights") {
    Network::getInstance(0)->learningRate = 1;

    for (int n=1; n<3; n++) {
        Network::getInstance(0)->layers[1]->neurons[n]->weights = {1,1,1};
        Network::getInstance(0)->layers[2]->neurons[n]->weights = {2,2,2};
        Network::getInstance(0)->layers[1]->neurons[n]->deltaWeights = {1,2,3};
        Network::getInstance(0)->layers[2]->neurons[n]->deltaWeights = {4,5,6};
    }

    std::vector<double> expected1 = {2,3,4};
    std::vector<double> expected2 = {6,7,8};

    Network::getInstance(0)->applyDeltaWeights();

    for (int n=1; n<3; n++) {
        REQUIRE( Network::getInstance(0)->layers[1]->neurons[n]->weights == expected1 );
        REQUIRE( Network::getInstance(0)->layers[2]->neurons[n]->weights == expected2 );
    }
}

/* Layer */
TEST_CASE("Layer::assignNext - Assigns the given layer pointer to this layer's nextLayer") {
    Layer* l1 = new Layer(0, 1);
    Layer* l2 = new Layer(0, 1);

    l1->assignNext(l2);

    delete l1;
    delete l2;
}

TEST_CASE("Layer::assignPrev - Assigns the given layer pointer to this layer's prevLayer") {
    Layer* l1 = new Layer(0, 1);
    Layer* l2 = new Layer(0, 1);

    l2->assignPrev(l1);

    delete l1;
    delete l2;
}

TEST_CASE("Layer::init - Fills the layers' neurons vectors with as many neurons as the layers' sizes") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 5);
    l2->assignPrev(l1);

    REQUIRE( l2->neurons.size() != 5 );
    l2->init(1);
    REQUIRE( l2->neurons.size() == 5 );

    delete l1;
    delete l2;
}

TEST_CASE("Layer::init - Sets the bias of every neuron to a number between -0.1 and 0.1") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 5);
    l2->assignPrev(l1);
    l2->init(1);

    for (int n=0; n<5; n++) {
        // REQUIRE(l2->neurons[n]->bias == 0);
        REQUIRE(l2->neurons[n]->bias >= -0.1);
        REQUIRE(l2->neurons[n]->bias <= 0.1);
    }

    delete l1;
    delete l2;
}

TEST_CASE("Layer::init - Inits the neurons' weights vector with as many weights as there are neurons in the prev layer. (none in first layer)") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 5);
    l2->assignPrev(l1);
    l1->init(0);
    l2->init(1);

    REQUIRE(l1->neurons[0]->weights.size() == 0);
    REQUIRE(l1->neurons[1]->weights.size() == 0);

    for (int n=0; n<5; n++) {
        REQUIRE(l2->neurons[n]->weights.size() == 2);
    }

    delete l1;
    delete l2;
}

TEST_CASE("Layer::forward - SETS the neurons' sum to their bias + weighted activations of last layer's neurons") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    l2->assignPrev(l1);
    l1->init(0);
    l2->init(1);
    l2->activation = &NetMath::sigmoid;

    l1->neurons[0]->activation = 1;
    l1->neurons[1]->activation = 2;

    for (int n=0; n<3; n++) {
        l2->neurons[n]->weights = {1,2};
        l2->neurons[n]->bias = n;
    }

    l2->forward();

    REQUIRE(l2->neurons[0]->sum == 5);
    REQUIRE(l2->neurons[1]->sum == 6);
    REQUIRE(l2->neurons[2]->sum == 7);

    // Check that it SETS it, and doesn't increment it
    l2->forward();

    REQUIRE(l2->neurons[0]->sum == 5);
    REQUIRE(l2->neurons[1]->sum == 6);
    REQUIRE(l2->neurons[2]->sum == 7);

    delete l1;
    delete l2;
}

TEST_CASE("Layer::forward - Sets the layer's neurons' activation to the result of the activation function") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    l2->assignPrev(l1);
    l1->init(0);
    l2->init(1);
    l2->activation = &NetMath::sigmoid;

    l1->neurons[0]->activation = 1;
    l1->neurons[1]->activation = 2;

    for (int n=0; n<3; n++) {
        l2->neurons[n]->weights = {1,2};
        l2->neurons[n]->bias = n;
    }

    l2->forward();

    REQUIRE(l2->neurons[0]->activation == 0.9933071490757153);
    REQUIRE(l2->neurons[1]->activation == 0.9975273768433653);
    REQUIRE(l2->neurons[2]->activation == 0.9990889488055994);

    delete l1;
    delete l2;
}

TEST_CASE("Layer::backward - Sets the neurons' errors to difference between their activations and expected values, when provided") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    l2->assignPrev(l1);
    l1->init(0);
    l2->init(1);

    std::vector<double> expected = {1,2,3};

    l2->neurons[0]->activation = 0;
    l2->neurons[1]->activation = 1;
    l2->neurons[2]->activation = 0;

    l2->backward(expected);

    REQUIRE( l2->neurons[0]->error == 1 );
    REQUIRE( l2->neurons[1]->error == 1 );
    REQUIRE( l2->neurons[2]->error == 3 );

    delete l1;
    delete l2;
}

TEST_CASE("Layer::backward - Sets the neurons' derivatives to the activation prime of their sum, when no expected data is passed") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    Layer* l3 = new Layer(0, 4);
    l2->assignPrev(l1);
    l2->assignNext(l3);
    l3->assignPrev(l2);
    l2->activation = &NetMath::sigmoid;
    l1->init(0);
    l2->init(1);
    l3->init(2);

    std::vector<double> emptyVec;

    l2->neurons[0]->sum = 0;
    l2->neurons[1]->sum = 1;
    l2->neurons[2]->sum = 0;
    l2->backward(emptyVec);

    REQUIRE( l2->neurons[0]->derivative == 0.25 );
    REQUIRE( l2->neurons[1]->derivative == 0.19661193324148185 );
    REQUIRE( l2->neurons[2]->derivative == 0.25 );

    delete l1;
    delete l2;
    delete l3;
}

TEST_CASE("Layer::backward - Sets the neurons' errors to their derivative multiplied by weighted errors in next layer, when no expected data is passed") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    Layer* l3 = new Layer(0, 4);
    l2->assignPrev(l1);
    l2->assignNext(l3);
    l3->assignPrev(l2);
    l2->activation = &NetMath::sigmoid;
    l1->init(0);
    l2->init(1);
    l3->init(2);

    std::vector<double> emptyVec;

    l2->neurons[0]->sum = 0.5;
    l2->neurons[1]->sum = 0.5;
    l2->neurons[2]->sum = 0.5;

    for (int i=0; i<4; i++) {
        l3->neurons[i]->error = 0.5;
        l3->neurons[i]->weights = {1,1,1,1};
    }

    l2->backward(emptyVec);

    REQUIRE( l2->neurons[0]->error == 0.470007424403189 );
    REQUIRE( l2->neurons[1]->error == 0.470007424403189 );
    REQUIRE( l2->neurons[2]->error == 0.470007424403189 );

    delete l1;
    delete l2;
    delete l3;
}

TEST_CASE("Layer::backward - Increments each of its delta weights by its error * the respective weight's neuron's activation") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    Layer* l3 = new Layer(0, 4);
    l2->assignPrev(l1);
    l2->assignNext(l3);
    l3->assignPrev(l2);
    l2->activation = &NetMath::sigmoid;
    l1->init(0);
    l2->init(1);
    l3->init(2);

    std::vector<double> expected = {1,2,3,4};

    l2->neurons[0]->activation = 0.5;
    l2->neurons[1]->activation = 0.5;
    l2->neurons[2]->activation = 0.5;

    for (int i=0; i<4; i++) {
        l3->neurons[i]->activation = 0.5;
    }

    l3->backward(expected);

    for (int n=0; n<4; n++) {
        REQUIRE( l3->neurons[n]->deltaWeights[0] == 0.25 + n * 0.5 );
        REQUIRE( l3->neurons[n]->deltaWeights[1] == 0.25 + n * 0.5 );
        REQUIRE( l3->neurons[n]->deltaWeights[2] == 0.25 + n * 0.5 );
    }

    delete l1;
    delete l2;
    delete l3;
}

TEST_CASE("Layer::backward - Sets the neurons' deltaBias to their errors") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    l2->assignPrev(l1);
    l1->init(0);
    l2->init(1);

    std::vector<double> expected = {1,2,3};

    l2->neurons[0]->activation = 0;
    l2->neurons[1]->activation = 1;
    l2->neurons[2]->activation = 0;

    l2->backward(expected);

    REQUIRE( l2->neurons[0]->error == 1 );
    REQUIRE( l2->neurons[0]->deltaBias == 1 );
    REQUIRE( l2->neurons[1]->error == 1 );
    REQUIRE( l2->neurons[1]->deltaBias == 1 );
    REQUIRE( l2->neurons[2]->error == 3 );
    REQUIRE( l2->neurons[2]->deltaBias == 3 );

    delete l1;
    delete l2;
}

TEST_CASE("Layer::applyDeltaWeights - Increments the weights by the delta weights") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    l2->assignPrev(l1);
    l2->init(1);

    for (int n=1; n<3; n++) {
        l2->neurons[n]->weights = {1,1,1};
        l2->neurons[n]->deltaWeights = {1,2,3};
    }

    l2->applyDeltaWeights();
    std::vector<double> expected = {2,3,4};

    for (int n=1; n<3; n++) {
        REQUIRE( l2->neurons[n]->weights == expected );
    }

    delete l1;
    delete l2;
}

TEST_CASE("Layer::applyDeltaWeights - Increments the bias by the deltaBias") {
    Network::getInstance(0)->learningRate = 1;
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    l2->assignPrev(l1);
    l2->init(1);

    // for (int n=1; n<3; n++) {
    for (int n=0; n<3; n++) {
        l2->neurons[n]->bias = n;
        l2->neurons[n]->deltaBias = n*2;
    }

    l2->applyDeltaWeights();

    REQUIRE( l2->neurons[0]->bias == 0 );
    REQUIRE( l2->neurons[1]->bias == 3 );
    REQUIRE( l2->neurons[2]->bias == 6 );

    delete l1;
    delete l2;
}

TEST_CASE("Layer::resetDeltaWeights - Sets all deltaWeight values to 0") {
    Layer* l1 = new Layer(0, 2);
    Layer* l2 = new Layer(0, 3);
    l2->assignPrev(l1);
    l2->init(1);

    for (int n=1; n<3; n++) {
        l2->neurons[n]->deltaWeights = {1,2,3};
    }
    std::vector<double> expected = {0,0,0};

    l2->resetDeltaWeights();

    for (int n=1; n<3; n++) {
        REQUIRE(l2->neurons[n]->deltaWeights == expected);
    }

    delete l1;
    delete l2;
}

/* Neuron */
TEST_CASE("Neuron::init - Fills the deltaWeights vector with 0 values, matching weights size") {
    Neuron* testN = new Neuron();
    testN->weights = {1,2,3,4,5};
    testN->init();
    REQUIRE( testN->deltaWeights.size() == 5 );
    delete testN;
}

TEST_CASE("Neuron::init - Sets the neuron deltaBias to 0") {
    Neuron* testN = new Neuron();
    testN->init();
    REQUIRE( testN->deltaBias == 0 );
    delete testN;
}

/* NetMath */
TEST_CASE("NetMath::sigmoid") {
    Neuron* testN = new Neuron();
    REQUIRE( NetMath::sigmoid(1.681241237, false, testN) == 0.8430688214048092 );
    REQUIRE( NetMath::sigmoid(0.8430688214048092, true, testN) == 0.21035474941074114 );
    delete testN;
}

TEST_CASE("NetMath::meansquarederror") {
    std::vector<double> values1 = {13,17,18,20,24};
    std::vector<double> values2 = {12,15,20,22,24};
    REQUIRE(NetMath::meansquarederror(values1, values2)==(double)2.6 );
}

TEST_CASE("NetMath::vanillaupdatefn") {
    Network* net = Network::getInstance(0);
    net->learningRate = 0.5;
    REQUIRE( NetMath::vanillaupdatefn(0, 10, 10)==15 );
    REQUIRE( NetMath::vanillaupdatefn(0, 10, 20)==20 );
    REQUIRE( NetMath::vanillaupdatefn(0, 10, -30)==-5 );
}
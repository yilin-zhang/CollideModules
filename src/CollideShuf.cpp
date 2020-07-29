#include <random>
#include "plugin.hpp"


struct StepsKnob : RoundSmallBlackKnob {
    StepsKnob() : RoundSmallBlackKnob() {
        snap = true;
    }
};

std::default_random_engine generator;
std::uniform_real_distribution<float> distribution (0.0f, 1.0f);

struct CollideShuf : Module {
    enum ParamIds {
        PARAM_WGT_1,
        PARAM_WGT_2,
        PARAM_WGT_3,
        PARAM_WGT_4,
        PARAM_WGT_5,
        PARAM_WGT_6,
        PARAM_WGT_7,
        PARAM_WGT_8,
        PARAM_STEPS,
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_WGT_1,
        INPUT_WGT_2,
        INPUT_WGT_3,
        INPUT_WGT_4,
        INPUT_WGT_5,
        INPUT_WGT_6,
        INPUT_WGT_7,
        INPUT_WGT_8,
        INPUT_CLK,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT_GATE_1,
        OUTPUT_GATE_2,
        OUTPUT_GATE_3,
        OUTPUT_GATE_4,
        OUTPUT_GATE_5,
        OUTPUT_GATE_6,
        OUTPUT_GATE_7,
        OUTPUT_GATE_8,
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(GATE_LIGHTS, 8),
        NUM_LIGHTS
    };

    dsp::SchmittTrigger clockTrigger;
    int numSteps = 8, clockGate=0;
    float weightInputs[8] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    float weights[8] = {0.125f, 0.125f, 0.125f, 0.125f, 0.125f, 0.125f, 0.125f, 0.125f};

    CollideShuf() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PARAM_STEPS, 1, 8, 8, "Steps");
        configParam(PARAM_WGT_1, 0.f, 1.f, 0.5f, "Weight 1");
        configParam(PARAM_WGT_2, 0.f, 1.f, 0.5f, "Weight 2");
        configParam(PARAM_WGT_3, 0.f, 1.f, 0.5f, "Weight 3");
        configParam(PARAM_WGT_4, 0.f, 1.f, 0.5f, "Weight 4");
        configParam(PARAM_WGT_5, 0.f, 1.f, 0.5f, "Weight 5");
        configParam(PARAM_WGT_6, 0.f, 1.f, 0.5f, "Weight 6");
        configParam(PARAM_WGT_7, 0.f, 1.f, 0.5f, "Weight 7");
        configParam(PARAM_WGT_8, 0.f, 1.f, 0.5f, "Weight 8");
    }

    void process(const ProcessArgs& args) override {
        int currentNumSteps = params[PARAM_STEPS].getValue();
        int updateWeightsFlag = false;
        float weightInput, clockInput, randValue, stepSum=0.f;

        // display lights based on steps
        if (numSteps != currentNumSteps) {
            // update numSteps
            numSteps = currentNumSteps;
            // refresh the red light setting
            for (int i=0; i<numSteps; ++i) {
                lights[GATE_LIGHTS + i].setBrightness(1.f);
            }
            for (int i=numSteps; i<8; ++i) {
                outputs[i].setVoltage(0.f); // set all the unused outputs 0
                lights[GATE_LIGHTS + i].setBrightness(0.f);
            }
            // set the flag true
            updateWeightsFlag = true;
        }

        // check if weights need to be updated
        for (int i=0; i<numSteps; ++i) {
            if (inputs[i].isConnected())
                // accept unipolar input
                weightInput = abs(inputs[i].getVoltage()) / 10.f;
            else
                weightInput = params[i].getValue();

            if (weightInputs[i] != weightInput) {
                // update weightInputs[i] and set the flag true
                weightInputs[i] = weightInput;
                updateWeightsFlag = true;
            }
        }

        if (updateWeightsFlag) {
            // update weights
            float sum = 0;
            for (int i=0; i<numSteps; ++i)
                sum += weightInputs[i];

            if (sum < 0.00001f) {
                for (int i=0; i<numSteps; ++i)
                    weights[i] = 0.125f;
            } else {
                for (int i=0; i<numSteps; ++i)
                    weights[i] = weightInputs[i] / sum;
            }
        }

        // check clock input
        clockInput = abs(inputs[INPUT_CLK].getVoltage()) / 10.f;
        if (clockTrigger.process(clockInput)) {
            clockGate = 1 - clockGate;
            if (clockGate == 1) {
                randValue = distribution(generator);
                for (int i=0; i<numSteps; ++i) {
                    if (randValue >= stepSum && randValue < (stepSum + weights[i])) {
                        outputs[i].setVoltage(10.f);
                    }
                    stepSum += weights[i];
                }
            } else {
                for (int i=0; i<numSteps; ++i) {
                    outputs[i].setVoltage(0.f);
                }
            }
        }
    }
};

struct CollideShufWidget : ModuleWidget {
    CollideShufWidget(CollideShuf* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CollideShuf.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<StepsKnob>(Vec(80.1, 90.7), module, CollideShuf::PARAM_STEPS));
        addParam(createParamCentered<Trimpot>(Vec(48.5, 120.2), module, CollideShuf::PARAM_WGT_1));
        addParam(createParamCentered<Trimpot>(Vec(48.5, 149.0), module, CollideShuf::PARAM_WGT_2));
        addParam(createParamCentered<Trimpot>(Vec(48.5, 177.6), module, CollideShuf::PARAM_WGT_3));
        addParam(createParamCentered<Trimpot>(Vec(48.5, 206.4), module, CollideShuf::PARAM_WGT_4));
        addParam(createParamCentered<Trimpot>(Vec(48.5, 235.1), module, CollideShuf::PARAM_WGT_5));
        addParam(createParamCentered<Trimpot>(Vec(48.5, 263.8), module, CollideShuf::PARAM_WGT_6));
        addParam(createParamCentered<Trimpot>(Vec(48.5, 292.7), module, CollideShuf::PARAM_WGT_7));
        addParam(createParamCentered<Trimpot>(Vec(48.5, 321.3), module, CollideShuf::PARAM_WGT_8));

        addInput(createInputCentered<PJ301MPort>(Vec(36, 90), module, CollideShuf::INPUT_CLK));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 120.2), module, CollideShuf::INPUT_WGT_1));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 149.0), module, CollideShuf::INPUT_WGT_2));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 177.6), module, CollideShuf::INPUT_WGT_3));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 206.4), module, CollideShuf::INPUT_WGT_4));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 235.1), module, CollideShuf::INPUT_WGT_5));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 263.8), module, CollideShuf::INPUT_WGT_6));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 292.7), module, CollideShuf::INPUT_WGT_7));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 321.3), module, CollideShuf::INPUT_WGT_8));

        addOutput(createOutputCentered<PJ301MPort>(Vec(92.9, 120.2), module, CollideShuf::OUTPUT_GATE_1));
        addOutput(createOutputCentered<PJ301MPort>(Vec(92.9, 149.0), module, CollideShuf::OUTPUT_GATE_2));
        addOutput(createOutputCentered<PJ301MPort>(Vec(92.9, 177.6), module, CollideShuf::OUTPUT_GATE_3));
        addOutput(createOutputCentered<PJ301MPort>(Vec(92.9, 206.4), module, CollideShuf::OUTPUT_GATE_4));
        addOutput(createOutputCentered<PJ301MPort>(Vec(92.9, 235.1), module, CollideShuf::OUTPUT_GATE_5));
        addOutput(createOutputCentered<PJ301MPort>(Vec(92.9, 263.8), module, CollideShuf::OUTPUT_GATE_6));
        addOutput(createOutputCentered<PJ301MPort>(Vec(92.9, 292.7), module, CollideShuf::OUTPUT_GATE_7));
        addOutput(createOutputCentered<PJ301MPort>(Vec(92.9, 321.3), module, CollideShuf::OUTPUT_GATE_8));

        addChild(createLightCentered<SmallLight<RedLight>>(Vec(110, 120.2), module, CollideShuf::GATE_LIGHTS + 0));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(110, 149.0), module, CollideShuf::GATE_LIGHTS + 1));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(110, 177.6), module, CollideShuf::GATE_LIGHTS + 2));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(110, 206.4), module, CollideShuf::GATE_LIGHTS + 3));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(110, 235.1), module, CollideShuf::GATE_LIGHTS + 4));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(110, 263.8), module, CollideShuf::GATE_LIGHTS + 5));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(110, 292.7), module, CollideShuf::GATE_LIGHTS + 6));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(110, 321.3), module, CollideShuf::GATE_LIGHTS + 7));
    }
};

Model* modelCollideShuf = createModel<CollideShuf, CollideShufWidget>("CollideShuf");

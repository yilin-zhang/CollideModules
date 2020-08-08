#include "plugin.hpp"
#include "ColliderUtils.h"

struct CollideFollow : Module {
    enum ParamIds {
        PARAM_SENSI_1,
        PARAM_SENSI_2,
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_SIGNAL_1,
        INPUT_SIGNAL_2,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT_SIGNAL_1,
        OUTPUT_SIGNAL_2,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    RCDiode<float> rcd_1, rcd_2;
    float env_1=0.f, env_2=0.f;

    CollideFollow():
    rcd_1(0.5*5),
    rcd_2(0.5*5)
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PARAM_SENSI_1, 0.f, 1.f, 0.5f, "Sensitivity 1");
        configParam(PARAM_SENSI_2, 0.f, 1.f, 0.5f, "Sensitivity 2");
    }

    void process(const ProcessArgs& args) override {
        float tau_1 = clamp((1 - params[PARAM_SENSI_1].getValue()) * 5, 0.01, 5.0);
        float tau_2 = clamp((1 - params[PARAM_SENSI_2].getValue()) * 5, 0.01, 5.0);

        float rect_in_1 = std::abs(inputs[INPUT_SIGNAL_1].getVoltage());
        float rect_in_2 = std::abs(inputs[INPUT_SIGNAL_2].getVoltage());

        rcd_1.setTau(tau_1);
        rcd_2.setTau(tau_2);

        if (rect_in_1 > env_1)
            env_1 = rcd_1.charge(rect_in_1);
        else
            env_1 = rcd_1.process(rect_in_1);

        if (rect_in_2 > env_2)
            env_2 = rcd_2.charge(rect_in_2);
        else
            env_2 = rcd_2.process(rect_in_2);

        if (outputs[OUTPUT_SIGNAL_1].isConnected())
            outputs[OUTPUT_SIGNAL_1].setVoltage(env_1);

        if (outputs[OUTPUT_SIGNAL_2].isConnected())
            outputs[OUTPUT_SIGNAL_2].setVoltage(env_2);
    }
};


struct CollideFollowWidget : ModuleWidget {
    CollideFollowWidget(CollideFollow* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CollideFollow.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // part 1
        addParam(createParamCentered<Rogan1PSWhite>(Vec(45, 115.9), module, CollideFollow::PARAM_SENSI_1));
        addInput(createInputCentered<PJ301MPort>(Vec(25.8, 173.1), module, CollideFollow::INPUT_SIGNAL_1));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64.2, 173.1), module, CollideFollow::OUTPUT_SIGNAL_1));

        // part 2
        addParam(createParamCentered<Rogan1PSWhite>(Vec(45, 250.9), module, CollideFollow::PARAM_SENSI_2));
        addInput(createInputCentered<PJ301MPort>(Vec(25.8, 308.1), module, CollideFollow::INPUT_SIGNAL_2));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64.2, 308.1), module, CollideFollow::OUTPUT_SIGNAL_2));
    }
};


Model* modelCollideFollow = createModel<CollideFollow, CollideFollowWidget>("CollideFollow");
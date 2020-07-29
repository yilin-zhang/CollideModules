#include "plugin.hpp"


struct CollidePan : Module {
	enum ParamIds {
		PARAM_PAN_1,
		PARAM_ATV_1,
        PARAM_PAN_2,
        PARAM_ATV_2,
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_SIGNAL_1,
		INPUT_MOD_1,
        INPUT_SIGNAL_2,
        INPUT_MOD_2,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT_SIGNAL_L_1,
		OUTPUT_SIGNAL_R_1,
        OUTPUT_SIGNAL_L_2,
        OUTPUT_SIGNAL_R_2,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    const int outIdxL[2] = {OUTPUT_SIGNAL_L_1, OUTPUT_SIGNAL_L_2};
    const int outIdxR[2] = {OUTPUT_SIGNAL_R_1, OUTPUT_SIGNAL_R_2};
    const int panIdx[2] = {PARAM_PAN_1, PARAM_PAN_2};
    const int atvIdx[2] = {PARAM_ATV_1, PARAM_ATV_2};
    const int modIdx[2] = {INPUT_MOD_1, INPUT_MOD_2};
    const int inIdx[2] = {INPUT_SIGNAL_1, INPUT_SIGNAL_2};

    float pan[2] = {0.5, 0.5};
    float atv[2] = {0, 0};

	CollidePan() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PARAM_PAN_1, -1.f, 1.f, 0.0f, "Pan 1");
		configParam(PARAM_ATV_1, -1.f, 1.f, 0.f, "Attenuverter 1");
        configParam(PARAM_PAN_2, -1.f, 1.f, 0.0f, "Pan 2");
        configParam(PARAM_ATV_2, -1.f, 1.f, 0.f, "Attenuverter 2");
	}

	void process(const ProcessArgs& args) override {

	    float out[2] = {0, 0};
	    float mod[2] = {0, 0};

	    for (int i=0; i<2; ++i) {
	        // if pan[i] and atv[i] don't change, don't update mod
	        if (inputs[inIdx[i]].isConnected()) {
	            out[i] = inputs[inIdx[i]].getVoltage();
                pan[i] = params[panIdx[i]].getValue();
                atv[i] = params[atvIdx[i]].getValue();

                // get pan modulation value
                if (inputs[modIdx[i]].isConnected()) {
                    mod[i] = inputs[modIdx[i]].getVoltage();
                    mod[i] = clamp(mod[i], -5.f, 5.f); // trim the value
                    mod[i] /= 5;
                }
                mod[i] = mod[i] * atv[i]; // range: (-1, 1)

                pan[i] = pan[i] + mod[i]; // modulate the pan
                pan[i] = clamp(pan[i], -1.f, 1.f); // trim the value
                pan[i] = (pan[i] + 1) / 2; // scale it to (0, 1)

                // equal power panning
                if (outputs[outIdxL[i]].isConnected()) {
                    outputs[outIdxL[i]].setVoltage(out[i] * sqrt(1 - pan[i]));
                }
                if (outputs[outIdxR[i]].isConnected()) {
                    outputs[outIdxR[i]].setVoltage(out[i] * sqrt(pan[i]));
                }
	        } else {
	            outputs[outIdxL[i]].setVoltage(0.f);
                outputs[outIdxR[i]].setVoltage(0.f);
	        }
	    }
    }
};


struct CollidePanWidget : ModuleWidget {
	CollidePanWidget(CollidePan* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CollidePan.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// part 1
		addParam(createParamCentered<Rogan1PSWhite>(Vec(63.5, 101.9), module, CollidePan::PARAM_PAN_1));
		addParam(createParamCentered<Trimpot>(Vec(63.5, 139), module, CollidePan::PARAM_ATV_1));

		addInput(createInputCentered<PJ301MPort>(Vec(25.8, 101.9), module, CollidePan::INPUT_SIGNAL_1));
		addInput(createInputCentered<PJ301MPort>(Vec(25.8, 139), module, CollidePan::INPUT_MOD_1));

		addOutput(createOutputCentered<PJ301MPort>(Vec(25.8, 184.5), module, CollidePan::OUTPUT_SIGNAL_L_1));
		addOutput(createOutputCentered<PJ301MPort>(Vec(64.2, 184.5), module, CollidePan::OUTPUT_SIGNAL_R_1));

		// part 2

        addParam(createParamCentered<Rogan1PSWhite>(Vec(63.5, 241.9), module, CollidePan::PARAM_PAN_2));
        addParam(createParamCentered<Trimpot>(Vec(63.5, 279), module, CollidePan::PARAM_ATV_2));

        addInput(createInputCentered<PJ301MPort>(Vec(25.8, 241.9), module, CollidePan::INPUT_SIGNAL_2));
        addInput(createInputCentered<PJ301MPort>(Vec(25.8, 279), module, CollidePan::INPUT_MOD_2));

        addOutput(createOutputCentered<PJ301MPort>(Vec(25.8, 324.5), module, CollidePan::OUTPUT_SIGNAL_L_2));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64.2, 324.5), module, CollidePan::OUTPUT_SIGNAL_R_2));
	}
};


Model* modelCollidePan = createModel<CollidePan, CollidePanWidget>("CollidePan");
#include "plugin.hpp"


struct CollidePan : Module {
	enum ParamIds {
		PARAM_PAN,
		PARAM_PAN_ATV,
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_SIGNAL,
		INPUT_PAN_MOD,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT_SIGNAL_L,
		OUTPUT_SIGNAL_R,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	CollidePan() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PARAM_PAN, -1.f, 1.f, 0.0f, "Pan");
		configParam(PARAM_PAN_ATV, -1.f, 1.f, 0.f, "Pan Attenuverter");
	}

	void process(const ProcessArgs& args) override {
	    float out, pan, panMod=0, panAtv;
	    if (inputs[INPUT_SIGNAL].isConnected()) {
            out = inputs[INPUT_SIGNAL].getVoltage();
	        pan = params[PARAM_PAN].getValue();
	        panAtv = params[PARAM_PAN_ATV].getValue();

	        // get pan modulation value
	        if (inputs[INPUT_PAN_MOD].isConnected()) {
                panMod = inputs[INPUT_PAN_MOD].getVoltage();

                panMod = clamp(panMod, -5.f, 5.f); // trim the value

                panMod /= 5;
            }
            panMod = panMod * panAtv; // range: (-1, 1)

	        pan = pan + panMod; // modulate the pan
            pan = clamp(pan, -1.f, 1.f); // trim the value
            pan = (pan + 1) / 2; // scale it to (0, 1)

            // equal power panning
            if (outputs[OUTPUT_SIGNAL_L].isConnected()) {
                outputs[OUTPUT_SIGNAL_L].setVoltage(out * sqrt(1 - pan));
            }
            if (outputs[OUTPUT_SIGNAL_R].isConnected()) {
                outputs[OUTPUT_SIGNAL_R].setVoltage(out * sqrt(pan));
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

		addParam(createParamCentered<Rogan2PSWhite>(Vec(45, 185.2), module, CollidePan::PARAM_PAN));
		addParam(createParamCentered<Trimpot>(Vec(64.9, 225), module, CollidePan::PARAM_PAN_ATV));

		addInput(createInputCentered<PJ301MPort>(Vec(45, 118.9), module, CollidePan::INPUT_SIGNAL));
		addInput(createInputCentered<PJ301MPort>(Vec(26.5, 225), module, CollidePan::INPUT_PAN_MOD));

		addOutput(createOutputCentered<PJ301MPort>(Vec(26.5, 299.9), module, CollidePan::OUTPUT_SIGNAL_L));
		addOutput(createOutputCentered<PJ301MPort>(Vec(64.9, 299.9), module, CollidePan::OUTPUT_SIGNAL_R));
	}
};


Model* modelCollidePan = createModel<CollidePan, CollidePanWidget>("CollidePan");
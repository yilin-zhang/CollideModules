#include "plugin.hpp"


struct CollidePan : Module {
	enum ParamIds {
		PAN_PARAM,
		PAN_ATV_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SIGNAL_INPUT,
		PAN_MOD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIGNAL_R_OUTPUT,
		SIGNAL_L_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	CollidePan() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PAN_PARAM, -1.f, 1.f, 0.0f, "PAN_PARAM");
		configParam(PAN_ATV_PARAM, -1.f, 1.f, 0.f, "PAN_ATV_PARAM");
	}

	void process(const ProcessArgs& args) override {
	    float sig, pan, panMod=0, panAtv;
	    if (inputs[SIGNAL_INPUT].isConnected()) {
            sig = inputs[SIGNAL_INPUT].getVoltage();
	        pan = params[PAN_PARAM].getValue();
	        panAtv = params[PAN_ATV_PARAM].getValue();

	        // get pan modulation value
	        if (inputs[PAN_MOD_INPUT].isConnected()) {
                panMod = inputs[PAN_MOD_INPUT].getVoltage();

                // trim the range
                if (panMod > 5)
                    panMod = 5;
                else if (panMod < -5)
                    panMod = -5;

                panMod /= 5;
            }
            panMod = panMod * panAtv; // range: (-1, 1)

            // modulate the pan
	        pan = pan + panMod;
            // trim the range
            if (pan > 1)
                pan = 1;
            else if (pan < -1)
                pan = -1;
            // scale pan to (0, 1)
            pan = (pan + 1) / 2;

            // equal power panning
            if (outputs[SIGNAL_L_OUTPUT].isConnected()) {
                outputs[SIGNAL_L_OUTPUT].setVoltage(sig * sqrt(1 - pan));
            }
            if (outputs[SIGNAL_R_OUTPUT].isConnected()) {
                outputs[SIGNAL_R_OUTPUT].setVoltage(sig * sqrt(pan));
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

		addParam(createParamCentered<RoundLargeBlackKnob>(Vec(45, 185), module, CollidePan::PAN_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(64.9, 225), module, CollidePan::PAN_ATV_PARAM));

		addInput(createInputCentered<PJ301MPort>(Vec(45, 118.9), module, CollidePan::SIGNAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(26.5, 225), module, CollidePan::PAN_MOD_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(Vec(26.5, 299.9), module, CollidePan::SIGNAL_R_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(64.9, 299.9), module, CollidePan::SIGNAL_L_OUTPUT));
	}
};


Model* modelCollidePan = createModel<CollidePan, CollidePanWidget>("CollidePan");
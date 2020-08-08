#include "plugin.hpp"
#include "ColliderUtils.h"


// time values in second
const float MIN_STAGE_TIME = 1e-3f;
const float MAX_STAGE_TIME = 10.f;
const float LAMBDA_BASE = MAX_STAGE_TIME / MIN_STAGE_TIME;
const float EPSILON = 1e-3f; // the threshold for being close enough


struct CollideEnv : Module {
    enum ParamIds {
        PARAM_GATE_TRIG_SWITCH,
        PARAM_GATE_TRIG_BTN,

        PARAM_ATTACK,
        PARAM_DECAY,
        PARAM_SUSTAIN,
        PARAM_RELEASE,
        PARAM_ATTACK_ATV,
        PARAM_DECAY_ATV,
        PARAM_SUSTAIN_ATV,
        PARAM_RELEASE_ATV,

        NUM_PARAMS
    };
    enum InputIds {
        INPUT_SIGNAL,
        INPUT_GATE_TRIG,
        INPUT_ATTACK_MOD,
        INPUT_DECAY_MOD,
        INPUT_SUSTAIN_MOD,
        INPUT_REELASE_MOD,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT_ATTACK_GATE,
        OUTPUT_DECAY_GATE,
        OUTPUT_SUSTAIN_GATE,
        OUTPUT_RELEASE_GATE,
        OUTPUT_ENV,
        OUTPUT_SIGNAL,
        OUTPUT_END,
        NUM_OUTPUTS
    };
    enum LightIds {
        LIGHT_ATTACK,
        LIGHT_DECAY,
        LIGHT_SUSTAIN,
        LIGHT_RELEASE,
        NUM_LIGHTS
    };

    enum Stages {
        STAGE_ATTACK,
        STAGE_DECAY,
        STAGE_SUSTAIN,
        STAGE_RELEASE,
        STAGE_END,
    };

    Stages stage;
    dsp::SchmittTrigger gateTrigger;
    dsp::PulseGenerator pulseGen;
    bool isActive;
    RCFilter<float> rcf;

    CollideEnv():
    rcf(pow(LAMBDA_BASE, 0.5) * MIN_STAGE_TIME)
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(PARAM_GATE_TRIG_SWITCH, 0, 1, 1, "Gate/Trig Switch");

        configParam(PARAM_ATTACK, 0.f, 1.f, 0.5f, "Attack", " ms", LAMBDA_BASE, MIN_STAGE_TIME * 1000);
        configParam(PARAM_DECAY, 0.f, 1.f, 0.5f, "Decay", " ms", LAMBDA_BASE, MIN_STAGE_TIME * 1000);
        configParam(PARAM_SUSTAIN, 0.f, 1.f, 0.5f, "Sustain", "%", 0, 100);
        configParam(PARAM_RELEASE, 0.f, 1.f, 0.5f, "Release", " ms", LAMBDA_BASE, MIN_STAGE_TIME * 1000);

        configParam(PARAM_ATTACK_ATV, -1.f, 1.f, 0.0f, "Attack Attenuverter");
        configParam(PARAM_DECAY_ATV, -1.f, 1.f, 0.0f, "Decay Attenuverter");
        configParam(PARAM_SUSTAIN_ATV, -1.f, 1.f, 0.0f, "Sustain Attenuverter");
        configParam(PARAM_RELEASE_ATV, -1.f, 1.f, 0.0f, "Release Attenuverter");

        isActive = false;
    }

    void process(const ProcessArgs& args) override {
        float inputGate = inputs[INPUT_GATE_TRIG].getVoltage();
        float attack, decay, sustain, release;
        float attackMod, attackAtv, decayMod, decayAtv, sustainMod, sustainAtv, releaseMod, releaseAtv;
        float Atau, Dtau, Sval, Rtau;
        bool boolGate = inputGate >= 1.0;
        float env;
        int mode;
        mode = params[PARAM_GATE_TRIG_SWITCH].getValue(); // 1: gate, 0: trig

        if (inputs[INPUT_GATE_TRIG].isConnected()) {
            if (params[PARAM_GATE_TRIG_BTN].getValue() == 1) {
                inputGate = 1.f;
                boolGate = true;
            } else {
                inputGate = inputs[INPUT_GATE_TRIG].getVoltage();
                boolGate = inputGate >= 1.0;
                inputGate /= 10.f;
            }
        } else {
            inputGate = params[PARAM_GATE_TRIG_BTN].getValue();
            boolGate = bool(inputGate);
        }

        attackAtv = params[PARAM_ATTACK_ATV].getValue();
        decayAtv = params[PARAM_DECAY_ATV].getValue();
        sustainAtv = params[PARAM_SUSTAIN_ATV].getValue();
        releaseAtv = params[PARAM_RELEASE_ATV].getValue();

        attackMod = clamp(inputs[INPUT_ATTACK_MOD].getVoltage() / 5.f, -1.f, 1.f);
        decayMod = clamp(inputs[INPUT_DECAY_MOD].getVoltage() / 5.f, -1.f, 1.f);
        sustainMod = clamp(inputs[INPUT_SUSTAIN_MOD].getVoltage() / 5.f, -1.f, 1.f);
        releaseMod = clamp(inputs[INPUT_REELASE_MOD].getVoltage() / 5.f, -1.f, 1.f);

        attack = params[PARAM_ATTACK].getValue();
        decay = params[PARAM_DECAY].getValue();
        sustain = params[PARAM_SUSTAIN].getValue();
        release = params[PARAM_RELEASE].getValue();

        Sval = clamp(sustain + sustainMod * sustainAtv, 0.f, 1.f);
        Atau = pow(LAMBDA_BASE, clamp(attack + attackMod * attackAtv, 0.f, 1.f)) * MIN_STAGE_TIME;
        Dtau = pow(LAMBDA_BASE, clamp(decay + decayMod * decayAtv, 0.f, 1.f)) * MIN_STAGE_TIME;
        Rtau = pow(LAMBDA_BASE, clamp(release + releaseMod * releaseAtv, 0.f, 1.f)) * MIN_STAGE_TIME;


        if (gateTrigger.process(inputGate)) {
            stage = STAGE_ATTACK;
            isActive = true;
        }

        // in gate mode, when the gate is 0, go to release stage
        if (mode == 1) {
            if ((!boolGate) && isActive) {
                stage = STAGE_RELEASE;
            }
        }

        outputs[OUTPUT_ATTACK_GATE].setVoltage(0.f);
        outputs[OUTPUT_DECAY_GATE].setVoltage(0.f);
        outputs[OUTPUT_SUSTAIN_GATE].setVoltage(0.f);
        outputs[OUTPUT_RELEASE_GATE].setVoltage(0.f);

        lights[LIGHT_ATTACK].setBrightness(0.f);
        lights[LIGHT_DECAY].setBrightness(0.f);
        lights[LIGHT_SUSTAIN].setBrightness(0.f);
        lights[LIGHT_RELEASE].setBrightness(0.f);

        if (isActive) {
            switch (stage) {
                case STAGE_ATTACK:
                    rcf.setTau(Atau);
                    env = rcf.process(1.0);
                    if (outputs[OUTPUT_ATTACK_GATE].isConnected()) {
                        outputs[OUTPUT_ATTACK_GATE].setVoltage(10.f);
                    }
                    lights[LIGHT_ATTACK].setBrightness(1.f);
                    if (abs(env - 1.0) <= EPSILON) {
                        if (mode == 1)
                            stage = STAGE_DECAY;
                        else
                            stage = STAGE_RELEASE; // jump to release in trig mode
                    }
                    break;
                case STAGE_DECAY:
                    rcf.setTau(Dtau);
                    env = rcf.process(Sval);
                    if (outputs[OUTPUT_DECAY_GATE].isConnected()) {
                        outputs[OUTPUT_DECAY_GATE].setVoltage(10.f);
                    }
                    lights[LIGHT_DECAY].setBrightness(1.f);
                    if (abs(env - Sval) <= EPSILON) {
                        stage = STAGE_SUSTAIN;
                    }
                    break;
                case STAGE_SUSTAIN:
                    env = Sval;
                    if (outputs[OUTPUT_SUSTAIN_GATE].isConnected()) {
                        outputs[OUTPUT_SUSTAIN_GATE].setVoltage(10.f);
                    }
                    lights[LIGHT_SUSTAIN].setBrightness(1.f);
                    break;
                case STAGE_RELEASE:
                    rcf.setTau(Rtau);
                    env = rcf.process(0.f);
                    if (outputs[OUTPUT_RELEASE_GATE].isConnected()) {
                        outputs[OUTPUT_RELEASE_GATE].setVoltage(10.f);
                    }
                    lights[LIGHT_RELEASE].setBrightness(1.f);
                    if (env <= EPSILON) {
                        stage = STAGE_END;
                        rcf.reset();
                    }
                    break;
                default:
                    env = 0.f;
            }
        } else {
            env = 0.f;
        }

        // outputs
        if (outputs[OUTPUT_ENV].isConnected()) {
            outputs[OUTPUT_ENV].setVoltage(env * 10.f);
        }

        if (inputs[INPUT_SIGNAL].isConnected() && outputs[OUTPUT_SIGNAL].isConnected()) {
            float in = inputs[INPUT_SIGNAL].getVoltage();
            outputs[OUTPUT_SIGNAL].setVoltage(in * env);
        }

        if (isActive && (stage == STAGE_END)) {
            // output end port
            isActive = false;
            pulseGen.trigger();
        }
        outputs[OUTPUT_END].setVoltage(pulseGen.process(args.sampleTime) * 10.f);
    }
};

struct CollideEnvWidget : ModuleWidget {
    CollideEnvWidget(CollideEnv* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CollideEnv.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // inputs
        addInput(createInputCentered<PJ301MPort>(Vec(25.8, 76.9), module, CollideEnv::INPUT_SIGNAL));
        addInput(createInputCentered<PJ301MPort>(Vec(61.0, 77.0), module, CollideEnv::INPUT_GATE_TRIG));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 126.6), module, CollideEnv::INPUT_ATTACK_MOD));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 174.6), module, CollideEnv::INPUT_DECAY_MOD));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 222.6), module, CollideEnv::INPUT_SUSTAIN_MOD));
        addInput(createInputCentered<PJ301MPort>(Vec(16.5, 270.6), module, CollideEnv::INPUT_REELASE_MOD));

        // params
        addParam(createParamCentered<CKD6>(Vec(97.7, 76.9), module, CollideEnv::PARAM_GATE_TRIG_BTN));
        addParam(createParamCentered<CKSS>(Vec(129.4, 76.9), module, CollideEnv::PARAM_GATE_TRIG_SWITCH));
        addParam(createParamCentered<Trimpot>(Vec(46.5, 126.6), module, CollideEnv::PARAM_ATTACK_ATV));
        addParam(createParamCentered<Trimpot>(Vec(46.5, 174.6), module, CollideEnv::PARAM_DECAY_ATV));
        addParam(createParamCentered<Trimpot>(Vec(46.5, 222.6), module, CollideEnv::PARAM_SUSTAIN_ATV));
        addParam(createParamCentered<Trimpot>(Vec(46.5, 270.6), module, CollideEnv::PARAM_RELEASE_ATV));
        addParam(createParamCentered<Rogan1PWhite>(Vec(81, 126.6), module, CollideEnv::PARAM_ATTACK));
        addParam(createParamCentered<Rogan1PWhite>(Vec(81, 174.6), module, CollideEnv::PARAM_DECAY));
        addParam(createParamCentered<Rogan1PWhite>(Vec(81, 222.6), module, CollideEnv::PARAM_SUSTAIN));
        addParam(createParamCentered<Rogan1PWhite>(Vec(81, 270.6), module, CollideEnv::PARAM_RELEASE));

        // outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(36.1, 329.6), module, CollideEnv::OUTPUT_ENV));
        addOutput(createOutputCentered<PJ301MPort>(Vec(74.5, 329.6), module, CollideEnv::OUTPUT_SIGNAL));
        addOutput(createOutputCentered<PJ301MPort>(Vec(112, 329.6), module, CollideEnv::OUTPUT_END));
        addOutput(createOutputCentered<PJ301MPort>(Vec(122.9, 126.6), module, CollideEnv::OUTPUT_ATTACK_GATE));
        addOutput(createOutputCentered<PJ301MPort>(Vec(122.9, 174.6), module, CollideEnv::OUTPUT_DECAY_GATE));
        addOutput(createOutputCentered<PJ301MPort>(Vec(122.9, 222.6), module, CollideEnv::OUTPUT_SUSTAIN_GATE));
        addOutput(createOutputCentered<PJ301MPort>(Vec(122.9, 270.6), module, CollideEnv::OUTPUT_RELEASE_GATE));

        // lights
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(140, 126.6), module, CollideEnv::LIGHT_ATTACK));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(140, 174.6), module, CollideEnv::LIGHT_DECAY));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(140, 222.6), module, CollideEnv::LIGHT_SUSTAIN));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(140, 270.6), module, CollideEnv::LIGHT_RELEASE));
    }
};

Model* modelCollideEnv = createModel<CollideEnv, CollideEnvWidget>("CollideEnv");

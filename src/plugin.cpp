#include "plugin.hpp"


Plugin *pluginInstance;

void init(Plugin *p) {
	pluginInstance = p;

	p->addModel(modelCollidePan);
    p->addModel(modelCollideShuf);
    p->addModel(modelCollideEnv);

}

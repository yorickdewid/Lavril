#ifndef _MODULES_H_
#define _MODULES_H_

/* Register external modules */
register_module(math);
register_module(system);
register_module(crypto);
register_module(curl);
register_module(json);
register_module(sqlite);
register_module(pgsql);

/* Modules places here are loaded at startup */
#define lv_init_modules(v) { \
	init_module(math, v); \
	init_module(system, v); \
	init_module(crypto, v); \
	init_module(curl, v); \
	init_module(json, v); \
}

#endif // _MODULES_H_

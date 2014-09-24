/*
 *
 * Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 */

#include <linux/module.h>

#include "source.h"

static int __init mod_init(void)
{ return 0; }

static void __exit mod_exit(void)
{ }

module_init(mod_init);
module_exit(mod_exit);

MODULE_DESCRIPTION("A RPI based module");

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Francesco Salvestrini <f.salvestrini@nextworks.it>");

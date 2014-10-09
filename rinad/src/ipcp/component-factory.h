#ifndef __COMPONENT_FACTORY_H__
#define __COMPONENT_FACTORY_H__

#include <string>

#include "ipcp/components.h"

namespace rinad {

extern "C" {
        typedef IPCProcessComponent *(*component_factory_create_t)(
                                                IPCProcess * ipc_process);
        typedef void (*component_factory_destroy_t)(
                                        IPCProcessComponent * ipc_process);
        typedef int (*plugin_init_function_t)(IPCProcess * ipc_process);
}

struct ComponentFactory {
        /* Name of this pluggable component. */
        std::string name;

        /* Name of the component where this plugin applies. */
        std::string component;

        /* Constructor method for instances of this pluggable component. */
        component_factory_create_t create;

        /* Destructor method for instances of this pluggable component. */
        component_factory_destroy_t destroy;
};

}

#endif  /* __COMPONENT_FACTORY_H__ */

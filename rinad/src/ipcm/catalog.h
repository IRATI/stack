#include <map>
#include <string>
#include <vector>

#include "librina/application.h"


// Data structure that represent a plugin in the catalog
struct CatalogPlugin {
	// The name of the plugin
	std::string name;

	// Is the plugin already loaded ?
	bool loaded;
};

struct CatalogPsInfo: public rina::PsInfo {
	// Back-reference to the plugin that published this
	// policy-set
	std::map<std::string, CatalogPlugin>::iterator plugin;

	// Is the policy-set alread loaded (i.e. is the associated
	// plugin already loaded ?
	bool loaded;

	CatalogPsInfo() : PsInfo() { }
	CatalogPsInfo(const std::string& n, const std::string& c,
	              const std::string& v);
};

class Catalog {
public:
	Catalog();

private:
	std::map<std::string, CatalogPsInfo> policy_sets;
	std::map<std::string, CatalogPlugin> plugins;
};

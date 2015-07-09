#include <iostream>
#include <string>

#include "catalog.h"

using namespace std;


CatalogPsInfo::CatalogPsInfo(const string& n, const string& c, const string& v)
				: PsInfo(n, c, v)
{
	loaded = false;
}

Catalog::Catalog()
{
}

/*
 * librina-cdap.cc
 *
 *  Created on: Mar 10, 2015
 *      Author: bernat
 */

#include "librina-wrap/cdap_provider.h"


class TestCDAPFactory
{

};

void main ()
{
  cdap::CDAPProviderFactory factory;
  cdap::CDAPProviderInterface *cdap_provider = factory.create("GPB", 2000);

}

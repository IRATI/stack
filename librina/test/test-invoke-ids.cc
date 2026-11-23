//
// Test invoke ids
//
//    Sergio Gimenez <sergio.gimenez@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <iostream>
#include <set>

#include "librina/cdap_v2.h"

using namespace rina;

static int fail(const char * what)
{
	std::cerr << "FAILED: " << what << std::endl;
	return 1;
}

int main()
{
	cdap_rib::concrete_syntax_t syntax;
	cdap::init(NULL, syntax, true);
	cdap::CDAPInvokeIdManager * ids =
		cdap::getProvider()->get_session_manager()->get_invoke_id_manager();

	int a = ids->newInvokeId(true);
	int b = ids->newInvokeId(true);
	if (a <= 0 || b <= 0 || a == b)
		return fail("invoke ids must be positive and distinct");

	// The reply to a's operation arrives, or the operation is cancelled: a is
	// free. Handing it straight to the next operation would let a late or
	// duplicate reply to a's operation complete that one instead.
	ids->freeInvokeId(a, false);
	int c = ids->newInvokeId(true);
	if (c == a || c == b)
		return fail("a freed invoke id was handed out again at once");

	// Many operations later the turn comes back round; ids still in use are
	// skipped and none is handed out twice.
	std::set<int> live;
	live.insert(b);
	live.insert(c);
	for (int i = 0; i < 1000; i++) {
		int id = ids->newInvokeId(true);
		if (id <= 0 || !live.insert(id).second)
			return fail("an invoke id in use was handed out again");
		if (i % 2 == 0) {
			ids->freeInvokeId(id, false);
			live.erase(id);
		}
	}

	std::cout << "Invoke ids are handed out in turn and never while in use"
		  << std::endl;
	return 0;
}

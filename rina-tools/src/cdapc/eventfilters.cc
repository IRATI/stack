//
// CDAP connector
//
// Micheal Crotty <mcrotty@tssg.org>
// Copyright (C) 2016, PRISTINE, Waterford Insitute of Technology
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#include <sstream>
#include <iostream>

#include "eventfilters.h"


using namespace std;

/**
 * EventFilter Implementation
 */

// Implementation of EventFilter
class EventFilterImpl : public ES_EventFilter {
	string name_;
	vector<string> matches;

public:
	EventFilterImpl(const string& name);
	EventFilterImpl(const string& name, const vector<string>& m);

	// Match
	bool match(const string& json);

	// Get the name of the filter
	const string& getName() {
		return name_;
	}
};

// Ctor
EventFilterImpl::EventFilterImpl(const string& name) {
	name_ = name;
}

EventFilterImpl::EventFilterImpl(const string& name, const vector<string>& m) {
	name_ = name;
	matches = m;
}

// Match everything that is in vector
bool EventFilterImpl::match(const string& json) {
	for(vector<string>::iterator it = matches.begin(); it != matches.end(); ++it) {
		size_t found = json.find(*it);
		if (found == string::npos) {
			//cout << "Failed on:" << *it << ":" << endl;
			//cout << "JSON:" << json << endl;
			return false;
		}
	}
	return true;
}  

/**
 * Builder 
 */

// Add a filter constraint
void ES_EventFilterBuilder::add(const char* key, const char* value) {
	stringstream ss;
	ss << "\"" << key << "\":\"";
	ss << value << "\"";
	matches.push_back(ss.str());
} 


// Create an event filter
ES_EventFilter* ES_EventFilterBuilder::build() {
	ES_EventFilter* filter;
	if (matches.size() > 0) {
		filter = new EventFilterImpl("constrained", matches);
	} else {
		filter = new EventFilterImpl("empty");
	}
	return filter;
}

// Reset
void ES_EventFilterBuilder::reset() {
	matches.clear();
	name_.clear();
}

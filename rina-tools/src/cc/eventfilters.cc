
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
      cout << "Failed on:" << *it << ":" << endl;
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

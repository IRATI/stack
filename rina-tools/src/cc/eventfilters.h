//
// CDAP Connector
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

#ifndef EVENTFILTERS_HPP
#define EVENTFILTERS_HPP

#include <vector>
#include <string>

#include "eventsystem.h"

/*
 * Used for filtering the JSON string
 */
class ES_EventFilter {

public:
  virtual bool match(const std::string& json) =0;

 // Get the naeme of the filter  
  virtual const std::string& getName() = 0;
};


/*
 * used to build filters
 */
class ES_EventFilterBuilder {
private:
  // A name
  std::string name_;
  // Attributes that must match
  //Map<std:string,std:string> matching = new Map<std::string, std::string>();
  std::vector<std::string> matches;    
  
public:
  
  /**
   * Allow construction
   */
  ES_EventFilterBuilder() {
    //matches = new std::vector<std::string>();
  }


  /**
   * Give the filter a name
  */
  ES_EventFilterBuilder& withName(const char* name){
		name_ = name;
		return *this;
	}
  
  /**
	 * Adds a filter for a TAX Source key. If the filter exists in the must-not-contain arguments it will be removed there.
	 * @param source new source to filter for
	 * @return self to allow chaining
	 */
	ES_EventFilterBuilder& fromSource(const char* source){
		add(HeaderKeys::TAX_SOURCE, source);
		return *this;
	}

  /**
	 * Adds a filter for a TAX Dialect key.
	 * If the filter exists in the must-not-contain arguments it will be removed there.
	 * @param dialect new dialect to filter for
	 * @return self to allow chaining
	 */
  ES_EventFilterBuilder& inDialect(const char * dialect){
      add (HeaderKeys::TAX_DIALECT, dialect);
  		return *this;
  }

	/**
	 * Adds a filter for a TAX Language key.
	 * If the filter exists in the must-not-contain arguments it will be removed there.
	 * @param language new language to filter for
	 * @return self to allow chaining
	 */
	ES_EventFilterBuilder& inLanguage(const char* language){
    add (HeaderKeys::TAX_LANGUAGE, language);
		return *this;
	}

  /**
	 * Adds a filter for a TAX Type key.
	 * If the filter exists in the must-not-contain arguments it will be removed there.
	 * @param type new type to filter for
	 * @return self to allow chaining
	 */
   ES_EventFilterBuilder& ofType(const char* type) {
     add(HeaderKeys::TAX_TYPE, type);
     return *this;
   };
     
   /**
 	 * Adds a filter for a TAX Version key.
 	 * If the filter exists in the must-not-contain arguments it will be removed there.
 	 * @param version new version to filter for
 	 * @return self to allow chaining
 	 */
   ES_EventFilterBuilder& ofVersion(const char* version) {
     add (HeaderKeys::TAX_VERSION, version);
   }
  
   /**
    * Create an event filter
    */
   ES_EventFilter* build();
   
   /**
    * Reset builder
    */
   void reset();
   
        
protected:
  // Add a filter constraint
  void add(const char* key, const char* value);
};

#endif // EVENTFILTERS_HPP

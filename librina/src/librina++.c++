//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "librina++.h++"
#include <iostream>

/* CLASS APPLICATION PROCESS NAMING INFORMATION */
ApplicationProcessNamingInformation::ApplicationProcessNamingInformation() {
}

ApplicationProcessNamingInformation::ApplicationProcessNamingInformation(
		const string& processName, const string& processInstance) {
	this->processName = processName;
	this->processInstance = processInstance;
}

const string& ApplicationProcessNamingInformation::getEntityInstance() const {
	return entityInstance;
}

void ApplicationProcessNamingInformation::setEntityInstance(
		const string& entityInstance) {
	this->entityInstance = entityInstance;
}

const string& ApplicationProcessNamingInformation::getEntityName() const {
	return entityName;
}

void ApplicationProcessNamingInformation::setEntityName(
		const string& entityName) {
	this->entityName = entityName;
}

const string& ApplicationProcessNamingInformation::getProcessInstance() const {
	return processInstance;
}

void ApplicationProcessNamingInformation::setProcessInstance(
		const string& processInstance) {
	this->processInstance = processInstance;
}

const string& ApplicationProcessNamingInformation::getProcessName() const {
	return processName;
}

void ApplicationProcessNamingInformation::setProcessName(
		const string& processName) {
	this->processName = processName;
}

/* CLASS FLOW SPECIFICATION */

FlowSpecification::FlowSpecification(){
	averageSDUBandwidth = 0;
	averageBandwidth = 0;
	peakBandwidthDuration = 0;
	peakSDUBandwidthDuration = 0;
	undetectedBitErrorRate = 0;
	partialDelivery = true;
	orderedDelivery = false;
	maxAllowableGap = -1;
	jitter = 0;
	delay = 0;
	maxSDUsize = 0;
}


unsigned int FlowSpecification::getAverageBandwidth() const {
	return averageBandwidth;
}

void FlowSpecification::setAverageBandwidth(unsigned int averageBandwidth) {
	this->averageBandwidth = averageBandwidth;
}

unsigned int FlowSpecification::getAverageSduBandwidth() const {
	return averageSDUBandwidth;
}

void FlowSpecification::setAverageSduBandwidth(unsigned int averageSduBandwidth) {
	averageSDUBandwidth = averageSduBandwidth;
}

unsigned int FlowSpecification::getDelay() const {
	return delay;
}

void FlowSpecification::setDelay(unsigned int delay) {
	this->delay = delay;
}

unsigned int FlowSpecification::getJitter() const {
	return jitter;
}

void FlowSpecification::setJitter(unsigned int jitter) {
	this->jitter = jitter;
}

int FlowSpecification::getMaxAllowableGap() const {
	return maxAllowableGap;
}

void FlowSpecification::setMaxAllowableGap(int maxAllowableGap) {
	this->maxAllowableGap = maxAllowableGap;
}

unsigned int FlowSpecification::getMaxSDUSize() const {
	return maxSDUsize;
}

void FlowSpecification::setMaxSDUSize(unsigned int maxSduSize) {
	maxSDUsize = maxSduSize;
}

bool FlowSpecification::isOrderedDelivery() const {
	return orderedDelivery;
}

void FlowSpecification::setOrderedDelivery(bool orderedDelivery) {
	this->orderedDelivery = orderedDelivery;
}

bool FlowSpecification::isPartialDelivery() const {
	return partialDelivery;
}

void FlowSpecification::setPartialDelivery(bool partialDelivery) {
	this->partialDelivery = partialDelivery;
}

unsigned int FlowSpecification::getPeakBandwidthDuration() const {
	return peakBandwidthDuration;
}

void FlowSpecification::setPeakBandwidthDuration(unsigned int peakBandwidthDuration) {
	this->peakBandwidthDuration = peakBandwidthDuration;
}

unsigned int FlowSpecification::getPeakSduBandwidthDuration() const {
	return peakSDUBandwidthDuration;
}

void FlowSpecification::setPeakSduBandwidthDuration(unsigned int peakSduBandwidthDuration) {
	peakSDUBandwidthDuration = peakSduBandwidthDuration;
}

double FlowSpecification::getUndetectedBitErrorRate() const {
	return undetectedBitErrorRate;
}

void FlowSpecification::setUndetectedBitErrorRate(double undetectedBitErrorRate) {
	this->undetectedBitErrorRate = undetectedBitErrorRate;
}

/* CLASS FLOW */

Flow::Flow(const ApplicationProcessNamingInformation& sourceApplication,
		const ApplicationProcessNamingInformation& destinationApplication,
		const FlowSpecification& flowSpecification){
	this->sourceApplication = sourceApplication;
	this->destinationApplication = destinationApplication;
	this->flowSpecification = flowSpecification;
	this->portId = 0;
}

void Flow::allocate() throw (exception){
	cout<<"Allocate flow called!";
	this->portId = 25;
}

void Flow::deallocate() throw (exception){
	cout<<"Deallocate flow called!";
	this->portId = 0;
}

int Flow::getPortId() const{
	return this->portId;
}

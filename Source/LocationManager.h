//
// LocationManager.h
//
// Clark Kromenaker
//
// Handles location-related tracking and logic.
// Knows what are considered valid locations, where different actors are,
// and how often they've been at those locations throughout the game.
//
#pragma once
#include <string>
#include <unordered_map>

#include "Type.h"

class Timeblock;

class LocationManager
{
	TYPE_DECL_BASE();
public:
	LocationManager();
	
	bool IsValidLocation(const std::string& locationCode) const;
	void DumpLocations() const;
	
	std::string GetLocation() const { return mLocation; }
	std::string GetLastLocation() const { return mLastLocation; }
	void SetLocation(const std::string& location);
    
    std::string GetLocationDisplayName() const;
    std::string GetLocationDisplayName(const std::string& location) const;
	
	int GetLocationCountAcrossAllTimeblocks(const std::string& actorName, const std::string& location);
	int GetCurrentLocationCountForCurrentTimeblock(const std::string& actorName) const;
	int GetLocationCountForCurrentTimeblock(const std::string& actorName, const std::string& location) const;
	int GetLocationCount(const std::string& actorName, const std::string& location, const Timeblock& timeblock) const;
	int GetLocationCount(const std::string& actorName, const std::string& location, const std::string& timeblock) const;
	
	void IncCurrentLocationCountForCurrentTimeblock(const std::string& actorName);
	void IncLocationCountForCurrentTimeblock(const std::string& actorName, const std::string& location);
	void IncLocationCount(const std::string& actorName, const std::string& location, const Timeblock& timeblock);
	void IncLocationCount(const std::string& actorName, const std::string& location, const std::string& timeblock);
	
	void SetLocationCountForCurrentTimeblock(const std::string& actorName, const std::string& location, int count);
	
	void SetActorLocation(const std::string& actorName, const std::string& location);
	std::string GetActorLocation(const std::string& actorName) const;
	
	void SetActorOffstage(const std::string& actorName);
	bool IsActorOffstage(const std::string& actorName) const;
	
private:
	// Maps a 3-letter "short" location code (e.g. din) to a "long" location code (e.g. Hotel_Dining_Room).
	// It's unclear what the "long" location code is for, but it is printed when you call DumpLocations().
	// This can be used to determine whether a loc code is valid (if it exists).
	std::unordered_map<std::string, std::string> mLocCodeShortToLocCodeLong;
	
	// Current and last location.
	std::string mLocation = "non";
	std::string mLastLocation = "non";
	
	// Location counts for actors. We track lifetime times an actor visits a location AND per-timeblock counts.
	// Key is actorName+location (e.g. gabrielr25) or actorName+locationId+timeblockCode (e.g. gabrielr25110a).
	// Value is number of times the actor has been at that location during that timeblock.
	std::unordered_map<std::string, int> mActorLocationCounts;
	std::unordered_map<std::string, int> mActorLocationTimeblockCounts;
	
	// A mapping of actor to location. If not present, the actor is "offstage".
	std::unordered_map<std::string, std::string> mActorLocations;
};

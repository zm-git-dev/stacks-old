//
//  StacksDocument.h
//  StacksGui3
//
//  Created by Nathan Dunn on 4/18/13.
//  Copyright (c) 2013 Nathan Dunn. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class DatumMO;
@class StackMO;
@class LocusMO;
@class PopulationMO;
@class DatumRepository;
@class PopulationRepository;
@class LocusRepository;
@class StackRepository;

@interface StacksDocument : NSPersistentDocument

@property(strong) NSString *path;
@property(atomic, retain) NSMutableDictionary *populationLookup;

@property(nonatomic, strong) NSSet *loci;
@property(nonatomic, strong) NSSet *populations;


// handle selections
@property(nonatomic, strong) LocusMO *selectedLocus;
@property(nonatomic, strong) PopulationMO *selectedPopulation;
@property(nonatomic, strong) NSArray *selectedDatums;
@property(nonatomic, strong) StackMO *selectedStack;

// repositories
@property(nonatomic, strong) DatumRepository *datumRepository ;
@property(nonatomic, strong) LocusRepository *locusRepository ;
@property(nonatomic, strong) PopulationRepository *populationRepository ;
@property(nonatomic, strong) StackRepository *stackRepository;



@property(nonatomic, copy) NSString *previousStacksName;

//- (NSMutableArray *)findPopulations;


- (NSManagedObjectContext *)getContextForPath:(NSString *)path;
- (NSManagedObjectContext *)getContextForPath:(NSString *)string andName:(NSString *)name;
@end

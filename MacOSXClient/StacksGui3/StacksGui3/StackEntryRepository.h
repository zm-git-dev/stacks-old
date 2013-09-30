//
// Created by Nathan Dunn on 5/8/13.
// Copyright (c) 2013 Nathan Dunn. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import <Foundation/Foundation.h>

@class StackEntryMO;
@class DatumMO;
@class ConsensusStackEntryMO;
@class ModelStackEntryMO;
@class ReferenceStackEntryMO;

@interface StackEntryRepository : NSObject
- (StackEntryMO *)insertStackEntry:(NSManagedObjectContext *)context entryId:(NSNumber *)id relationship:(NSString *)relationship block:(NSString *)block sequenceId:(NSString *)sequenceId sequence:(NSString *)sequence datum:(DatumMO *)datum;

- (ConsensusStackEntryMO *)insertConsensusStackEntry:(NSManagedObjectContext *)context block:(id)block sequenceId:(id)id1 sequence:(id)sequence datum:(DatumMO *)datum;

- (ModelStackEntryMO *)insertModelStackEntry:(NSManagedObjectContext *)context block:(NSString *)block sequenceId:(NSString *)sequenceId sequence:(id)sequence datum:(DatumMO *)datum;

- (ReferenceStackEntryMO *)insertReferenceStackEntry:(NSManagedObjectContext *)context sequence:(NSString *)sequence datum:(DatumMO *)datum;
@end
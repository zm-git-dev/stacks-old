//
// Created by Nathan Dunn on 5/8/13.
// Copyright (c) 2013 Nathan Dunn. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import <Foundation/Foundation.h>

@class StackEntryMO;
@class StackMO;

@interface StackEntryRepository : NSObject
- (StackEntryMO *)insertStackEntry:(NSManagedObjectContext *)context entryId:(NSNumber *)id relationship:(NSString *)relationship block:(NSString *)block sequenceId:(NSString *)sequenceId sequence:(NSString *)sequence stack:(StackMO *)stack;
@end
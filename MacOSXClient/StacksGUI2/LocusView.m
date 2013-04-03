//
//  LocusView.m
//  StacksGUI
//
//  Created by Nathan Dunn on 2/11/13.
//  Copyright (c) 2013 University of Oregon. All rights reserved.
//

#import "TreeProtocol.h"
#import "LocusView.h"
#import "GenotypeView.h"
#import "StacksView.h"
#import "GenotypeEntry.h"

@implementation LocusView
@synthesize depth ;
@synthesize genotypes;


- (id)initWithId:(NSString *)locusId {
    if ((self = [super init])) {
        self.locusId = locusId;
    }
    return self;
}

//@synthesize locusId;
//@synthesize consensus;
//@synthesize marker;
//@synthesize genotypes;
//@synthesize progeny;
//@synthesize ratio;
//@synthesize snps;

// TODO: fix parental functions
// to identify the # of parents: look at "identify_parents" in genotypes.cc .
- (NSInteger)matchingParents {
    NSInteger count = 0;
//    if (_male) {
//        ++count;
//    }
//    if (_female) {
//        ++count;
//    }
    return count;
}

//- (NSInteger)genotypes {
//    return [_progeny count];
//}

- (BOOL)hasMale {
//    if (_male !=nil ){
//        return (_male.superScript!= nil || _male.subScript !=nil);
//    }
    return FALSE;
}

- (BOOL)hasFemale {
//    if (_female !=nil ){
//        return (_female.superScript!= nil || _female.subScript !=nil);
//    }
    return FALSE;
}

- (NSInteger)count {
    return self.genotypeCount;
//    return self.matchingParents + self.genotypes;
}

- (NSUInteger)childCount {
    if(genotypes== nil){
        return 0 ;
    }
    return genotypes.count;
}

- (id)childAtIndex:(NSUInteger)index1 {
//    NSArray *sortedKeys = [self.locusViews.allKeys sortedArrayUsingComparator:(NSComparator) ^(id obj1, id obj2) {
//        return [obj1 integerValue] - [obj2 integerValue];
//    }];
//    NSString *key = [sortedKeys objectAtIndexedSubscript:index];
//    return [self.locusViews objectForKey:key];
    NSArray *sortedKeys = [self.genotypes.allKeys sortedArrayUsingComparator:(NSComparator) ^(id obj1, id obj2) {
//        return [obj1 integerValue] - [obj2 integerValue];
        return [obj1 compare:obj2];
    }];

//    for(id i in genotypes.allKeys){
//        NSLog(@"@sorted array %@%",i);
//    }


    NSString *key = [sortedKeys objectAtIndexedSubscript:index1];
    GenotypeEntry *genotypeEntry = [genotypes objectForKey:key];
    return genotypeEntry;
}

- (BOOL)isLeaf {
    return YES ;
}


@end
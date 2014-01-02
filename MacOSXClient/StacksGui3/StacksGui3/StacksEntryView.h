//
// Created by Nathan Dunn on 12/20/13.
// Copyright (c) 2013 Nathan Dunn. All rights reserved.
//


#import <Foundation/Foundation.h>


@interface StacksEntryView : NSObject
@property(nonatomic) NSInteger locusId;
@property(nonatomic, copy) NSString *sampleName;
@property(nonatomic, copy) NSString* consensus;
@property(nonatomic, copy) NSString*  model;
@property(nonatomic, copy) NSMutableArray*  sequenceIds;
@property(nonatomic, copy) NSMutableArray*  sequences;

- (NSString *)renderHtml;
@end
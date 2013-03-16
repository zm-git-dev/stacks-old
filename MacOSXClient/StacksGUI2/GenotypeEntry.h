//
// Created by ndunn on 2/28/13.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import <Foundation/Foundation.h>


@interface GenotypeEntry : NSObject

@property NSString *name ;
@property NSInteger entryId ;
//@property NSString *type;

// TODO: remove
@property NSString *superScript;
@property NSString *subScript;

// replaced with this:
@property (strong) NSMutableArray *haplotypes;
@property (strong) NSMutableArray *depths;


// link info
@property NSInteger  *sampleId;
@property NSInteger  *tagId;

- (NSString *)render;
@end
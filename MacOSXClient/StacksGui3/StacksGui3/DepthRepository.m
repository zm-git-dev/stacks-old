//
// Created by Nathan Dunn on 5/8/13.
// Copyright (c) 2013 Nathan Dunn. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//


#import "DepthRepository.h"
#import "DepthMO.h"


@implementation DepthRepository {

}
- (DepthMO *)insertDepth:(NSManagedObjectContext *)context depth:(NSNumber *)depth andOrder:(int)order {
    DepthMO *depthMO = [NSEntityDescription insertNewObjectForEntityForName:@"Depth" inManagedObjectContext:context];
    depthMO.depth = depth ;
    depthMO.order = [NSNumber numberWithInt:order];
    return depthMO ;
}
@end
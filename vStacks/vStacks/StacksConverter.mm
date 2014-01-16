//
// Created by NathanDunn on 2/28/13.
//
//



#include "LocusMO.h"
//#include "stacks.h"
#include "sql_utilities.h"
#include "PopMap.h"
#import "StacksDocument.h"


#include <fstream>

using std::ifstream;
using std::ofstream;


//#import "StackEntryMO.h"
#import "SampleRepository.h"
#import "StacksConverter.h"
#import "PopulationRepository.h"
//#import "DepthRepository.h"
#import "DatumRepository.h"
//#import "HaplotypeRepository.h"
#import "LocusRepository.h"
#import "DatumMO.h"
//#import "HaplotypeMO.h"
//#import "DepthMO.h"
#import "PopulationMO.h"
#import "SampleMO.h"
//#import "SnpRepository.h"
//#import "StackEntryRepository.h"
//#import "DatumSnpMO.h"
//#import "DatumAlleleMO.h"
//#import "AlleleRepository.h"
//#import "LocusAlleleMO.h"
//#import "ConsensusStackEntryMO.h"
#import "ProgressController.h"
#import "StackEntryRenderer.h"
#import "GZIP.h"
#import "StackEntryDatumMO.h"


#include <sys/time.h>


#define CHECK_STOP  if (stopProcess) { [progressWindow close]; return nil ; }


NSString *calculateType(NSString *file);

//void setParentCounts(NSMutableDictionary *dictionary, NSString *file);
//void setParentCounts(NSString *file);


@implementation StacksConverter {

}


/**
* https://casspr.fogbugz.com/default.asp?1144
* 1 - add pops
2 - add samples
3 - load locis (refactor into other method)
4 - create datum from loci and sample (do per sample, load loci into a lookupTable to use throughout)
5 - add stackEntries onto each datum per tags file
6 - load snps onto each datum
7 - load alleles onto each datum
*/

// repositories
//@synthesize [DatumRepository sharedInstance];
//@synthesize [DepthRepository sharedInstance];
//@synthesize [HaplotypeRepository sharedInstance];
//@synthesize [LocusRepository sharedInstance];
//@synthesize [PopulationRepository sharedInstance];
//@synthesize [SampleRepository sharedInstance];
//@synthesize [SnpRepository sharedInstance];
//@synthesize stackEntryRepository;
//@synthesize [AlleleRepository sharedInstance];


// lookups
//@synthesize lociDictionary;
@synthesize sampleLookupDictionary;
@synthesize stopProcess;
@synthesize locusSnpMap;
@synthesize numberFormatter;


@synthesize persistentStoreCoordinator;


- (id)init {
    self = [super init];
    if (self) {
        sampleLookupDictionary = [NSMutableDictionary dictionary];
        stopProcess = false;
        numberFormatter = [[NSNumberFormatter alloc] init];
        numberFormatter.numberStyle = NSNumberFormatterNoStyle;


        persistentStoreCoordinator = [[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:[NSManagedObjectModel mergedModelFromBundles:nil]];
    }
    return self;
}

- (NSString *)generateFilePathForUrl:(NSURL *)url {
    return [url.path stringByAppendingFormat:@"/%@.stacks", url.path.lastPathComponent];
}


- (StacksDocument *)loadLociAndGenotypes:(NSString *)path progressWindow:(ProgressController *)progressController {

    StacksDocument *stacksDocument = [self createStacksDocumentForPath:path];
    if (stacksDocument == nil) {
        return nil;
    }

    stacksDocument = [self loadDocument:stacksDocument progressWindow:progressController];
    NSManagedObjectContext *moc = stacksDocument.managedObjectContext;
    NSError *error;
    [moc save:&error];
    NSLog(@"error %@", error);
    [[moc parentContext] save:&error];
    NSLog(@"error 2 %@", error);
//    [moc reset];
//    [[moc parentContext] save:&error];
//    [[moc parentContext] performBlock:^(){
//        [[moc parentContext] save:NULL];
//    }];

//    [lociDictionary removeAllObjects];
//    [sampleLookupDictionary removeAllObjects];
//    [moc reset];
//    [[moc parentContext] reset];


    return stacksDocument;
}

//- (NSManagedObjectContext *)getContextForPath:(NSString *)path andDocument:(StacksDocument *)document{
//    if(document.name==NULL){
//        return [self getContextForPath:path andName:@"StacksDocument" andDocument:document];
//    }
//    else{
//
//        return [self getContextForPath:path andName:document.name  andDocument:document];
//    }
//}


- (NSManagedObjectContext *)getContextForPath:(NSString *)path andName:(NSString *)name andDocument:(StacksDocument *)document {
    NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:YES], NSMigratePersistentStoresAutomaticallyOption,
                                                                       [NSNumber numberWithBool:YES],
                                                                       NSInferMappingModelAutomaticallyOption, nil];


    NSURL *storeUrl = [NSURL fileURLWithPath:[path stringByAppendingFormat:@"/%@.stacks", name]];
    NSLog(@"saving to %@ from %@", path, storeUrl);
    if (persistentStoreCoordinator == nil) {
        persistentStoreCoordinator = [[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:[NSManagedObjectModel mergedModelFromBundles:nil]];
    }
    NSError *error = nil;

    if (![persistentStoreCoordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:nil URL:storeUrl options:options error:&error]) {
        NSLog(@"error loading persistent store..");
        [[NSFileManager defaultManager] removeItemAtPath:storeUrl.path error:nil];
        if (![persistentStoreCoordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:nil URL:storeUrl options:options error:&error]) {
            NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
            //abort();
        }
    }


    NSManagedObjectContext *context = [[NSManagedObjectContext alloc] init];
    [context setPersistentStoreCoordinator:persistentStoreCoordinator];
    document.managedObjectContext = context;
    document.path = path;


    return context;
}


- (NSMutableDictionary *)loadPopulation:(NSString *)path {
    NSMutableDictionary *populationLookup = [NSMutableDictionary dictionary];

    NSString *popmapFile = [path stringByAppendingString:@"popmap"];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    BOOL exists = [fileManager fileExistsAtPath:popmapFile];

    if (exists) {
        NSArray *fileData = [[NSString stringWithContentsOfFile:popmapFile encoding:NSUTF8StringEncoding error:nil] componentsSeparatedByString:@"\n"];
        NSString *line;
        for (line in fileData) {
            NSArray *columns = [line componentsSeparatedByString:@"\t"];
            if (columns.count == 2) {
                [populationLookup setObject:[columns objectAtIndex:1] forKey:[columns objectAtIndex:0]];
            }
            else {
                NSLog(@"something wrong with the column count %@", line);
            }
        }
    }
    else {
        NSLog(@"does not exist at %@", popmapFile);
    }

    return populationLookup;
}

/**
* returns all of the files that end with .tags.tsv in the directory
*/
- (vector<pair<int, string> >)buildFileList:(NSString *)path {
    vector<pair<int, string> > files;

    NSFileManager *fileManager = [NSFileManager defaultManager];

    NSDirectoryEnumerator *dirEnum = [fileManager enumeratorAtPath:path];
    NSString *file;
    int i = 0;
    while (file = [dirEnum nextObject]) {

        if ([file hasSuffix:@".tags.tsv"]) {
//            NSLog(@"HAS SUFFIX name %@", file);
            NSUInteger length = [file length] - 9;
            NSString *fileName = [file substringToIndex:length];
            files.push_back(make_pair(i, [fileName UTF8String]));
            ++i;
        }
    }

    sort(files.begin(), files.end());

    return files;
}

/**
* Exist on error.
*/
- (NSString *)checkFile:(NSString *)examplePath {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    BOOL existsAtPath = [fileManager fileExistsAtPath:examplePath];

    if (!existsAtPath) {
        NSLog(@"files do not exist %@", examplePath);
        exit(1);
    }

    NSError *error;
    NSArray *files = [fileManager contentsOfDirectoryAtPath:examplePath error:&error];
    if (error) {
        NSLog(@"error %@", error);
        return nil;
    }
    NSPredicate *fltr = [NSPredicate predicateWithFormat:@"self ENDSWITH '.catalog.tags.tsv'"];
    NSPredicate *batch = [NSPredicate predicateWithFormat:@"self BEGINSWITH 'batch'"];
    NSArray *onlyCatalog = [[files filteredArrayUsingPredicate:fltr] filteredArrayUsingPredicate:batch];

    NSLog(@"# of files: %ld", onlyCatalog.count);
    for (NSString *file in onlyCatalog) {
        NSLog(@"file %@", file);
    }

    NSString *originalFile = [onlyCatalog objectAtIndex:0];
    NSUInteger firstIndex = [originalFile rangeOfString:@"."].location;
    NSString *batchName = [originalFile substringToIndex:firstIndex];

    return batchName;

}

- (StacksDocument *)loadDocument:(StacksDocument *)stacksDocument progressWindow:(ProgressController *)progressWindow {
    NSProgressIndicator *bar = progressWindow.loadProgress;
    if (bar != nil) {
        progressWindow.actionTitle.stringValue = [NSString stringWithFormat:@"Loading %@", stacksDocument.name];
        progressWindow.actionMessage.stringValue = @"Begin import";
        bar.doubleValue = 0;
        [bar display];
//        [bar incrementBy:1];
    }
    NSString *path = stacksDocument.path;
    // returns batch_1, batch_2, etc. whatever exists
    NSString *batchName = [self checkFile:path];
    NSLog(@"returned batch name: %@", batchName);
    map<int, CSLocus *> catalog;
    NSString *catalogTagFile = [path stringByAppendingFormat:@"%@.catalog.tags.tsv", batchName];
    stacksDocument.type = calculateType(catalogTagFile);
    NSLog(@"type is %@", stacksDocument.type);
    NSString *catalogFile = [path stringByAppendingFormat:@"%@.catalog", batchName];

    struct timeval time1, time2;
    gettimeofday(&time1, NULL);


    load_loci([catalogFile UTF8String], catalog, false);
    progressWindow.actionMessage.stringValue = @"Loading loci";
//    [bar incrementBy:2];
    gettimeofday(&time2, NULL);
    NSLog(@"load_loci %ld", (time2.tv_sec - time1.tv_sec));

    /**
    * START: loading datums + extra info
*/
    vector<vector<CatMatch *> > catalog_matches;
    map<int, string> samples;
    vector<int> sample_ids;

    progressWindow.actionMessage.stringValue = @"Building file list";
    vector<pair<int, string>> files = [self buildFileList:path];
//    [bar incrementBy:2];
    NSLog(@"number of files %ld", files.size());



    // loci loaded . . . now loading datum
    progressWindow.actionMessage.stringValue = @"Matching catalogs";
    NSLog(@"model size %d", (int) catalog.size());

    double incrementAmount = 5.0 / files.size();
    gettimeofday(&time1, NULL);
    @autoreleasepool {
    for (uint i = 0; i < files.size(); i++) {
        @autoreleasepool {
        vector<CatMatch *> m;
        NSString *sampleString = [NSString stringWithUTF8String:files[i].second.c_str()];
        NSString *matchString = [path stringByAppendingFormat:@"/%@", sampleString];
//        NSLog(@"loading match file %@ for sample name %@", matchString, sampleString);
        if (([sampleString rangeOfString:@"catalog"]).location == NSNotFound) {
            NSMutableDictionary *matchDictionary = [self loadMatchesDictionary:[matchString stringByAppendingString:@".matches.tsv"]];
            [sampleLookupDictionary setObject:matchDictionary forKey:sampleString];
        }

        load_catalog_matches([matchString UTF8String], m);

        if (m.size() == 0) {
            cerr << "Warning: unable to find any matches in file '" << files[i].second << "', excluding this sample from population analysis.\n";
            continue;
        }

        catalog_matches.push_back(m);
        if (samples.count(m[0]->sample_id) == 0) {
            samples[m[0]->sample_id] = files[i].second;
            sample_ids.push_back(m[0]->sample_id);
        } else {
            cerr << "Fatal error: sample ID " << m[0]->sample_id << " occurs twice in this stacksDocuments set, likely the pipeline was run incorrectly.\n";
            exit(1);
        }
        [bar incrementBy:incrementAmount];
        }
    }
    }
    // end of for
    gettimeofday(&time2, NULL);
    NSLog(@"catalog matches %ld", (time2.tv_sec - time1.tv_sec));


    NSLog(@"input populations %ld", stacksDocument.populations.count);
    progressWindow.actionMessage.stringValue = @"Adding populations";
    [self addPopulationsToDocument:stacksDocument forPath:path];
    [bar incrementBy:5];
    progressWindow.actionMessage.stringValue = @"Adding samples";
    NSLog(@"output populations %ld", stacksDocument.populations.count);
    [self addSamplesToDocument:stacksDocument forSampleIds:sample_ids andSamples:samples];
    [bar incrementBy:5];
    NSLog(@"output populations after samples %ld", stacksDocument.populations.count);
    for (PopulationMO *populationMo in stacksDocument.populations) {
        NSLog(@"samples %ld per population %@", populationMo.samples.count, populationMo.name);
//        for(SampleMO* sampleMO in populationMo.samples){
//            NSLog(@"sample %@ ",sampleMO.name);
//        }
    }

    cerr << "Populating observed haplotypes for " << sample_ids.size() << " samples, " << catalog.size() << " loci.\n";

    gettimeofday(&time1, NULL);
    progressWindow.actionMessage.stringValue = @"Populating samples";
    PopMap<CSLocus> *pmap = new PopMap<CSLocus>((int) sample_ids.size(), (int) catalog.size());
    pmap->populate(sample_ids, catalog, catalog_matches);

//    delete catalog_matches;
//    catalog_matches.erase (catalog_matches.begin(),catalog_matches.end());


    [bar incrementBy:5];
    gettimeofday(&time2, NULL);
    NSLog(@"population pmap %ld", (time2.tv_sec - time1.tv_sec));
    progressWindow.actionMessage.stringValue = @"Populated Samples";


    NSMutableSet *loci = [NSMutableSet set];

    gettimeofday(&time1, NULL);
    map<int, CSLocus *>::iterator catalogIterator = catalog.begin();
    NSManagedObjectContext *moc = stacksDocument.managedObjectContext;


    incrementAmount = 10.0 / catalog.size();

    progressWindow.actionMessage.stringValue = @"Loading locus snps";
    @autoreleasepool {
        while (catalogIterator != catalog.end()) {
            @autoreleasepool {
            const char *read = (*catalogIterator).second->con;
            LocusMO *locusMO = [[LocusRepository sharedInstance] insertNewLocus:moc withId:[NSNumber numberWithInt:(*catalogIterator).second->id]
                                                                   andConsensus:[[NSString alloc] initWithCString:read encoding:NSUTF8StringEncoding] andMarker:[NSString stringWithUTF8String:catalogIterator->second->marker.c_str()]
            ];

            // get catalogs for matches
            // TODO: double-check that this is correct . . .
            locusMO.length = [NSNumber numberWithInt:catalogIterator->second->depth];
            locusMO.type = stacksDocument.type;

//        NSLog(@"chromosme %@",[NSString stringWithUTF8String:catalogIterator->second->loc.chr]);

            locusMO.chromosome = [NSString stringWithUTF8String:catalogIterator->second->loc.chr];
            unsigned int intValue = (unsigned int) catalogIterator->second->loc.bp;
            locusMO.basePairs = [NSNumber numberWithUnsignedInt:intValue];
            locusMO.strand = catalogIterator->second->loc.strand == plus ? @"+" : @"-";

            @autoreleasepool {
                vector<SNP *> snps = catalogIterator->second->snps;
                vector<SNP *>::iterator snpsIterator = snps.begin();


                NSMutableArray *snpArray = [NSMutableArray array];
                for (; snpsIterator != snps.end(); ++snpsIterator) {
                    SNP *snp = (*snpsIterator);
                    @autoreleasepool {
                        NSDictionary *snpDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                [NSString stringWithFormat:@"%i", snp->col], @"column"
                                , [NSNumber numberWithFloat:snp->lratio], @"lratio"
                                , [NSNumber numberWithFloat:snp->rank_1], @"rank1"
                                , [NSNumber numberWithFloat:snp->rank_2], @"rank2"
                                , [NSNumber numberWithFloat:snp->rank_3], @"rank3"
                                , [NSNumber numberWithFloat:snp->rank_4], @"rank4"
                                , nil ];
                        [snpArray addObject:snpDictionary];
                    }

                }

                @autoreleasepool {
                    NSError *error2;
                    locusMO.snpData = [NSJSONSerialization dataWithJSONObject:snpArray options:0 error:&error2];
                }
            }


            @autoreleasepool {
                map<string, int> alleles = catalogIterator->second->alleles;
                map<string, int>::iterator allelesIterator = alleles.begin();
                NSMutableArray *alleleArray = [NSMutableArray array];
                for (; allelesIterator != alleles.end(); ++allelesIterator) {
                    string allele = allelesIterator->first;
                    int column = allelesIterator->second;


                    @autoreleasepool {

                        NSDictionary *alleleDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                [NSNumber numberWithInt:column], @"depth"
                                , [NSString stringWithUTF8String:allele.c_str()], @"allele"
//                    ,[NSNumber numberWithFloat:snp->rank_1],@"ratio"
                                , nil ];
                        [alleleArray addObject:alleleDictionary];

                        NSError *error2;
                        locusMO.alleleData = [NSJSONSerialization dataWithJSONObject:alleleArray options:0 error:&error2];
                    }
                }
            }


            [loci addObject:locusMO];
            ++catalogIterator;
            [bar incrementBy:incrementAmount];
        }
        }
    }
    gettimeofday(&time2, NULL);


    [self setParentCounts:stacksDocument.managedObjectContext forFile:catalogTagFile loci:loci];

    NSLog(@"populating snps %ld", (time2.tv_sec - time1.tv_sec));

    map<int, CSLocus *>::iterator it;
    Datum *datum;
    CSLocus *loc;


    CHECK_STOP
    progressWindow.actionMessage.stringValue = @"Loading datums";
    // for each sample process the catalog
    NSLog(@"samples %ld X catalog %ld = %ld ", sample_ids.size(), catalog.size(), sample_ids.size() * catalog.size());

    long totalCatalogTime = 0;
    incrementAmount = 30.0 / (sample_ids.size() * catalog.size());

    // 7 is 400 X 7 = 3K . . .
    uint saveAfterSamples = 10000;


    gettimeofday(&time1, NULL);

    long iterCount = 0;
    //go through all samples
    for (uint i = 0; i < sample_ids.size(); i++) {
        int sampleId = sample_ids[i];
        CHECK_STOP
        @autoreleasepool {

            string sampleString = samples[sampleId];
            progressWindow.actionMessage.stringValue = [NSString stringWithFormat:@"Loading datum %i/%ld", i + 1, sample_ids.size()];

            NSString *key = [NSString stringWithUTF8String:sampleString.c_str()];

            gettimeofday(&time1, NULL);
            // go through all loci
            for (it = catalog.begin(); it != catalog.end(); it++, iterCount++) {
                loc = it->second;
                datum = pmap->datum(loc->id, sample_ids[i]);

                if (datum != NULL) {

                    @autoreleasepool {


                        vector<char *> obshape = datum->obshap;
                        vector<int> depths = datum->depth;
                        int numLetters = (int) obshape.size();
                        DatumMO *newDatumMO = [NSEntityDescription insertNewObjectForEntityForName:@"Datum" inManagedObjectContext:moc];
                        newDatumMO.name = key;
                        newDatumMO.sampleId = [NSNumber numberWithInt:sampleId];
                        newDatumMO.tagId = [NSNumber numberWithInt:loc->id];

                        if (newDatumMO.sampleId == nil) {
                            NSLog(@"loading sample ID %@", newDatumMO.sampleId);
                        }

                        // TODO: CONVERT TO USE DATA
                        NSMutableArray *datumDataArray = [NSMutableArray array];
                        if (depths.size() == numLetters && numLetters > 0) {
                            for (int j = 0; j < numLetters; j++) {
                                @autoreleasepool {
                                    NSDictionary *dataDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                            [NSString stringWithUTF8String:obshape[j]], @"haplotype"
                                            , [NSNumber numberWithInt:j], @"order"
                                            , [NSNumber numberWithInt:depths[j]], @"depth"
                                            , nil ];
                                    [datumDataArray addObject:dataDictionary];
                                }
                            }

                            NSError *error;
                            @autoreleasepool {
                                newDatumMO.haplotypeData = [NSJSONSerialization dataWithJSONObject:datumDataArray options:0 error:&error];;
                            }
                        }
                        else {
                            NSLog(@"mismatchon %@", [NSString stringWithUTF8String:sampleString.c_str()]);
                        }

                    }
                }

                // end of process loci from catalogs

                if (iterCount % saveAfterSamples == 0) {
                    NSError *innerError = nil ;
                    NSLog(@"saving samples");
                    [stacksDocument.managedObjectContext save:&innerError];
                    if (innerError != nil) {
                        NSLog(@"error doing inner save: %@", innerError);
                        return nil;
                    }
                }
            }
        }


        NSError *innerError = nil ;
        NSLog(@"final datum save ");
        [stacksDocument.managedObjectContext save:&innerError];
        if (innerError != nil) {
            NSLog(@"error doing inner save: %@", innerError);
            return nil;
        }


        gettimeofday(&time2, NULL);
        totalCatalogTime += time2.tv_sec - time1.tv_sec;


        [bar incrementBy:incrementAmount];

    }

    @autoreleasepool {
    NSArray *allDatums = [[DatumRepository sharedInstance] getAllDatum:moc];
    for (DatumMO *datumMO in allDatums) {
        [moc refreshObject:datumMO mergeChanges:YES];
    }
    allDatums = nil ;
    }


    NSError *innerError = nil ;
    stacksDocument.loci = loci;
//    [loci removeAllObjects];
    loci = nil ;
    CHECK_STOP
    progressWindow.actionMessage.stringValue = @"Saving doc";
    [stacksDocument.managedObjectContext save:&innerError];
    [bar incrementBy:5];
    if (innerError != nil) {
        NSLog(@"error doing inner save: %@", innerError);
        return nil;
    }

    NSLog(@"loci on doc %ld", stacksDocument.loci.count);
    for (LocusMO *locus in stacksDocument.loci) {
        [stacksDocument.managedObjectContext refreshObject:locus mergeChanges:YES];
    }

    NSLog(@"total time %ld", totalCatalogTime);

    gettimeofday(&time1, NULL);
    delete pmap;
    gettimeofday(&time2, NULL);
    NSLog(@"delete pmap time %ld", time2.tv_sec - time1.tv_sec);


    gettimeofday(&time1, NULL);
    // TODO: I don't think this does anything hear
    CHECK_STOP
    progressWindow.actionMessage.stringValue = @"Reading populations";
    [self readPopulations:stacksDocument];

    gettimeofday(&time2, NULL);
    NSLog(@"create stacks document time %ld", time2.tv_sec - time1.tv_sec);

    NSLog(@"loading snps onto datums");
    gettimeofday(&time1, NULL);
    progressWindow.actionMessage.stringValue = @"Loading datum snps";
    [self loadSnpsOntoDatum:stacksDocument progressWindow:progressWindow];
    gettimeofday(&time2, NULL);
    NSLog(@"finished loading snps onto datum time %ld", time2.tv_sec - time1.tv_sec);

    NSLog(@"loading alleles onto datums");
    gettimeofday(&time1, NULL);
    progressWindow.actionMessage.stringValue = @"Loading datum alleles";
    [self loadAllelesOntoDatum:stacksDocument];
    gettimeofday(&time2, NULL);
    NSLog(@"finished loading alleles onto datum time %ld", time2.tv_sec - time1.tv_sec);

    progressWindow.actionMessage.stringValue = @"Loading progeny counts";
    gettimeofday(&time1, NULL);


    NSDictionary *progenyDictionary = [[LocusRepository sharedInstance] getAggregateProgenyCount:moc];
    for (LocusMO *locus in [[LocusRepository sharedInstance] getAllLoci:moc]) {
        locus.progenyCount = [progenyDictionary objectForKey:locus.locusId];
//        locus.progenyCount = [[LocusRepository sharedInstance] getProgenyCount:moc  locus:locus];
    }

    progressWindow.actionMessage.stringValue = @"Saving doc";
    [stacksDocument.managedObjectContext save:&innerError];
//    [bar incrementBy:5];
    if (innerError != nil) {
        NSLog(@"error doing inner save: %@", innerError);
        return nil;
    }
//    NSLog(@"loci on doc %ld",stacksDocument.loci.count);
//    for(LocusMO* locus in stacksDocument.loci){
//        [stacksDocument.managedObjectContext refreshObject:locus mergeChanges:YES];
//    }

    gettimeofday(&time2, NULL);
    NSLog(@"progeny count time: %ld", time2.tv_sec - time1.tv_sec);

//    [stacksDocument.managedObjectContext reset];


    NSLog(@"loading stack entries");
    CHECK_STOP

//    for (id key in [sampleLookupDictionary allKeys]) {
//        NSLog(@"keys 1: %@", key);
//    }
    progressWindow.actionMessage.stringValue = @"Loading stack entries";
    gettimeofday(&time1, NULL);
//    [self loadStacksEntriesFromTagFile:stacksDocument];
    [self loadStacksEntriesFromTagFile:stacksDocument progressWindow:progressWindow];
    CHECK_STOP
    gettimeofday(&time2, NULL);
    NSLog(@"finished loading stacks entries time %ld", time2.tv_sec - time1.tv_sec);


    NSError *error;
    progressWindow.actionMessage.stringValue = @"Final save";
    [stacksDocument.managedObjectContext save:&error];
//    NSLog(@"saved %d", success);
    [bar incrementBy:5];

    return stacksDocument;
}


- (void)loadSnpsOntoDatum:(StacksDocument *)document progressWindow:(ProgressController *)window {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *path = document.path;

    // 1- get all files i the directory named *.tag.tsv"
    NSError *error;
    NSArray *files = [fileManager contentsOfDirectoryAtPath:path error:&error];
    NSLog(@"# of files for directory %ld", files.count);

    NSUInteger count = 0;
    // 2 - for each file, read the .tags file
    @autoreleasepool {
    for (NSString *filePath in files) {
        if ([filePath hasSuffix:@".snps.tsv"] && ![filePath hasPrefix:@"batch"]) {
            [self loadSnpFileForDatum:document fromFile:filePath];
        }
        else {
//            NSLog(@"not loading tag file %@", filePath);
        }
        window.actionMessage.stringValue = [NSString stringWithFormat:@"Loading datum snp %ld/%ld", count, files.count];
        ++count;
    }
    }
}

- (void)loadAllelesOntoDatum:(StacksDocument *)document {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *path = document.path;

    // 1- get all files i the directory named *.tag.tsv"
    NSError *error;
    NSArray *files = [fileManager contentsOfDirectoryAtPath:path error:&error];
    NSLog(@"# of files for directory %ld", files.count);

    // 2 - for each file, read the .tags file
    for (NSString *filePath in files) {
        if ([filePath hasSuffix:@".alleles.tsv"] && ![filePath hasPrefix:@"batch"]) {
            @autoreleasepool {
            [self loadAlleleFileForDatum:document fromFile:filePath];
            }
        }
        else {
//            NSLog(@"not loading alleles file %@", filePath);
        }
    }
}

- (void)loadAlleleFileForDatum:(StacksDocument *)document fromFile:(NSString *)alleleFileName {
//    NSLog(@"Loading allele file %@", alleleFileName);

    NSUInteger fileNameLength = alleleFileName.length;
    NSString *sampleName = [alleleFileName substringToIndex:fileNameLength - 12];
//    NSLog(@"sampleName %@", sampleName);
    // sampleName . . . from lsat index of "/" . . . to just before ".tags.tsv"

    NSManagedObjectContext *moc = document.managedObjectContext;
    SampleMO *sampleMO = [[SampleRepository sharedInstance] getSampleForName:sampleName andContext:document.managedObjectContext andError:nil];

    struct timeval time1, time2;
    gettimeofday(&time1, NULL);

    NSError *error2 = nil;
    NSString *absoluteFileName = [document.path stringByAppendingFormat:@"/%@", alleleFileName];
    NSArray *fileData = [[NSString stringWithContentsOfFile:absoluteFileName encoding:NSUTF8StringEncoding error:&error2] componentsSeparatedByString:@"\n"];
    gettimeofday(&time2, NULL);
//    NSLog(@"load file data and split %ld", (time2.tv_sec - time1.tv_sec));

    if (error2) {
        NSLog(@"error loading file [%@]: %@", alleleFileName, error2);
    }

    NSString *line;
//    NSUInteger row = 1;
    gettimeofday(&time1, NULL);
    NSInteger locusId = -1;
    NSInteger newLocusId;
    DatumMO *datumMO = nil ;
    NSMutableDictionary *lookupDictionary = [sampleLookupDictionary objectForKey:sampleName];
//    char allele;
    int depth;
    float ratio;
//    LocusMO *locusMO = nil ;

    @autoreleasepool {
    NSDictionary *datumLociMap = [[DatumRepository sharedInstance] getDatums:document.managedObjectContext forSample:sampleMO.sampleId];
    for (line in fileData) {
        NSArray *columns = [line componentsSeparatedByString:@"\t"];

        if (columns.count > 4) {

            // if the StackMO is found
//            sampleId = [[columns objectAtIndex:1] integerValue];
//            newLocusId = [[columns objectAtIndex:2] integerValue];

           
            @autoreleasepool {
            NSString *internalIndex = (NSString *) [columns objectAtIndex:2];
            NSString *retrievedObject = (NSString *) [lookupDictionary objectForKey:internalIndex];
            newLocusId = [retrievedObject integerValue];


//            allele = [[columns objectAtIndex:3] charValue];
//            allele = [numberFormatter numberFromString:[columns objectAtIndex:3]];
            ratio = [[columns objectAtIndex:4] floatValue];
            depth = [[columns objectAtIndex:5] intValue];


            if (locusId != newLocusId) {
                locusId = newLocusId;
//                datumMO = [[DatumRepository sharedInstance] getDatum:moc locusId:locusId andSampleId:sampleMO.sampleId.integerValue];

                datumMO = [datumLociMap objectForKey:[NSString stringWithFormat:@"%ld", locusId]];
            }
            }

//            if (datumMO != nil) {
//                // TODO: convert
////                [[AlleleRepository sharedInstance] insertDatumAllele:moc
////                                              ratio:[NSNumber numberWithFloat:ratio]
////                                              depth:[NSNumber numberWithInt:depth]
////                                             allele:[numberFormatter numberFromString:[columns objectAtIndex:3]]
////                                              datum:datumMO
////                ];
//            }
        }
    }
    }
    gettimeofday(&time2, NULL);


    // save old
    NSError *saveError;
    [moc save:&saveError];
//    NSLog(@"saved %d", success);
    if (saveError != nil ) {
        NSLog(@"error saving %@", saveError);
    }
}

- (void)loadSnpFileForDatum:(StacksDocument *)document fromFile:(NSString *)snpFileName {
//    NSLog(@"Loading snps file %@", snpFileName);

    NSUInteger fileNameLength = snpFileName.length;
    NSString *sampleName = [snpFileName substringToIndex:fileNameLength - 9];
    NSMutableDictionary *lookupDictionary = [sampleLookupDictionary objectForKey:sampleName];

    NSManagedObjectContext *moc = document.managedObjectContext;
    SampleMO *sampleMO = [[SampleRepository sharedInstance] getSampleForName:sampleName andContext:document.managedObjectContext andError:nil];

    struct timeval time1, time2;
    gettimeofday(&time1, NULL);

    NSError *error2 = nil;
    NSString *absoluteFileName = [document.path stringByAppendingFormat:@"/%@", snpFileName];
    NSArray *fileData = [[NSString stringWithContentsOfFile:absoluteFileName encoding:NSUTF8StringEncoding error:&error2] componentsSeparatedByString:@"\n"];
    gettimeofday(&time2, NULL);
//    NSLog(@"load file data and split %ld", (time2.tv_sec - time1.tv_sec));

    if (error2) {
        NSLog(@"error loading file [%@]: %@", snpFileName, error2);
    }

    NSString *line;
    gettimeofday(&time1, NULL);
    NSInteger locusId = -1;
    NSInteger newLocusId;
    NSInteger column;
    float lratio;
    DatumMO *datumMO = nil ;

    NSDictionary *datumLociMap = [[DatumRepository sharedInstance] getDatums:document.managedObjectContext forSample:sampleMO.sampleId];


    @autoreleasepool {
    NSMutableArray *snpArray;

//    NSDictionary *snpLociMap = [self getLocusSnpsForDocument:document];
    for (line in fileData) {
        @autoreleasepool {

            NSArray *columns = [line componentsSeparatedByString:@"\t"];

            if (columns.count > 6) {

                // if the StackMO is found
//            sampleId = [[columns objectAtIndex:1] integerValue];
//            newLocusId = [[columns objectAtIndex:2] integerValue];

                @autoreleasepool {
                NSString *internalIndex = (NSString *) [columns objectAtIndex:2];
                NSString *retrievedObject = (NSString *) [lookupDictionary objectForKey:internalIndex];
//            NSLog(@"retrieved object: %@", retrievedObject);
                newLocusId = [retrievedObject integerValue];

                column = [[columns objectAtIndex:3] integerValue];
                lratio = [[columns objectAtIndex:4] floatValue];


                if (locusId != newLocusId) {
                    locusId = newLocusId;
                    datumMO = [datumLociMap objectForKey:[NSString stringWithFormat:@"%ld", locusId]];
                }

                if (datumMO != nil) {

                    @autoreleasepool {
                        NSDictionary *snpDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                [NSNumber numberWithInteger:column], @"column"
                                , [NSNumber numberWithFloat:lratio], @"lratio"
                                , [numberFormatter numberFromString:[columns objectAtIndex:5]], @"rank1"
                                , [numberFormatter numberFromString:[columns objectAtIndex:6]], @"rank2"
                                , [numberFormatter numberFromString:[columns objectAtIndex:7]], @"rank3"
                                , [numberFormatter numberFromString:[columns objectAtIndex:8]], @"rank4"
                                , nil ];

                        if (datumMO.snpData == nil) {
                            snpArray = [NSMutableArray array];
                        }
                        else {
                            @autoreleasepool {
                            snpArray = [NSMutableArray arrayWithArray:[NSJSONSerialization JSONObjectWithData:datumMO.snpData options:kNilOptions error:&error2]];
                            }
                        }

                        @autoreleasepool {
                        [snpArray addObject:snpDictionary];
                        datumMO.snpData = [NSJSONSerialization dataWithJSONObject:snpArray options:0 error:&error2];
                        }
                    }


                }
                datumMO = nil ;
            }
            }
        }
    }

    snpArray = nil ;
        
    gettimeofday(&time2, NULL);
    }


    // save old
    NSError *saveError;
    [moc save:&saveError];
    if (saveError != nil ) {
        NSLog(@"error saving %@", saveError);
    }

}

- (NSMutableDictionary *)loadMatchesDictionary:(NSString *)name {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:name]) return nil;

    NSError *error2;
    NSArray *fileData = [[NSString stringWithContentsOfFile:name encoding:NSUTF8StringEncoding error:&error2] componentsSeparatedByString:@"\n"];
    NSMutableDictionary *lookupDictionary = [NSMutableDictionary dictionary];
    NSString *line;
    for (line in fileData) {
        NSArray *columns = [line componentsSeparatedByString:@"\t"];
        if (columns.count > 5) {
//        NSLog(@"column 2 values [%@] - [%@]",[columns objectAtIndex:2],[columns objectAtIndex:4]);
            NSString *internalId = [columns objectAtIndex:4];
            NSString *externalId = [columns objectAtIndex:2];
            [lookupDictionary setObject:externalId forKey:internalId];
        }
    }

    return lookupDictionary;
}

- (void)loadStacksEntriesFromTagFile:(StacksDocument *)document progressWindow:(ProgressController *)progressWindow {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *path = document.path;
    // TODO: if male . . .male.tags.tsv / female.tags.tsv
    // TODO: or *|!male|!female_N.tags.tsv

    // 1- get all files i the directory named *.tag.tsv"
    NSError *error;
//    NSArray *files = [fileManager contentsOfDirectoryAtPath:path error:&error];
    NSArray *files = [fileManager contentsOfDirectoryAtPath:path error:&error];
    NSMutableArray *realFiles = [NSMutableArray array];

    for (NSString *filePath in files) {
        if ([filePath hasSuffix:@".tags.tsv"] && ![filePath hasPrefix:@"batch"]) {
            [realFiles addObject:filePath];
        }
    }

    NSLog(@"# of files for directory %ld", files.count);


    // 2 - for each file, read the .tags file
    int fileNumber = 0;
    NSUInteger numFiles = realFiles.count;
    double incrementAmount = 30.0 / numFiles;

    struct timeval time1, time2;
    gettimeofday(&time1, NULL);

    NSUInteger saveCounter = 1;


    int size;
    char *line;
    ifstream fh;
    vector<string> parts;
    line = new char [max_len];
    size = max_len;
    long int line_num;
    long int totalLineNum = 0;
    long int totalSaves = 0 ;

    struct timeval time3, time4;

    for (NSString *tagFileName in realFiles) {
        progressWindow.actionMessage.stringValue = [NSString stringWithFormat:@"Loading stack entry %i / %ld", fileNumber + 1, numFiles];
        if (stopProcess) return;


        if ([tagFileName hasSuffix:@".tags.tsv"] && ![tagFileName hasPrefix:@"batch"]) {
//            CHECK_STOP
//            [self loadTagFile:document fromFile:filePath];

            NSLog(@"Loading tag file %@", tagFileName);

            NSUInteger fileNameLength = tagFileName.length;
            NSString *sampleName = [tagFileName substringToIndex:fileNameLength - 9];
//    NSLog(@"sampleName %@", sampleName);
            // sampleName . . . from lsat index of "/" . . . to just before ".tags.tsv"

            NSManagedObjectContext *moc = document.managedObjectContext;
            SampleMO *sampleMO = [[SampleRepository sharedInstance] getSampleForName:sampleName andContext:document.managedObjectContext andError:nil];


//            NSString *line;
//            NSUInteger row = 1;
            gettimeofday(&time3, NULL);
            NSInteger locusId = -1;
            NSInteger newLocusId = 0;

            StackEntryDatumMO *stackEntryDatumMO = nil ;


            NSMutableDictionary *lookupDictionary = [sampleLookupDictionary objectForKey:sampleName];

            NSString *absoluteFileName = [document.path stringByAppendingFormat:@"/%@", tagFileName];
            string f = [absoluteFileName cStringUsingEncoding:NSUTF8StringEncoding];

            fh.open(f.c_str(), ifstream::in);
            line_num = 0;

            if (fh.fail()) {
                NSLog(@"unable to open file: %@", absoluteFileName);

            }
            else {
                NSLog(@"opening good file: %@", absoluteFileName);

//                NSMutableArray *stackEntryArray = [NSMutableArray array];
                NSMutableDictionary *stackEntryDictionary = [NSMutableDictionary dictionary];
                int row = 1;

                @autoreleasepool {
                    while (fh.good()) {

                        read_line(fh, &line, &size);

                        if (fh.good() && strlen(line) > 0) {
                            parse_tsv(line, parts);

                            if (parts.size() != num_tags_fields) {
                                cerr << "Error parsing " << f.c_str() << " at line: " << line_num << ". (" << parts.size() << " fields).\n";
                                NSLog(@"error Parings %ld -> %ld", line_num, parts.size());
                                return;
                            }

                            @autoreleasepool {
                                NSString *internalIndex = [NSString stringWithUTF8String:parts[2].c_str()];
                                NSString *retrievedObject = (NSString *) [lookupDictionary objectForKey:internalIndex];

                                newLocusId = [retrievedObject integerValue];
                                if (locusId != newLocusId) {

                                    if (stackEntryDatumMO != nil) {
                                        @autoreleasepool {
                                            stackEntryDatumMO.stackData = [[NSJSONSerialization dataWithJSONObject:stackEntryDictionary options:0 error:&error] gzippedData];

                                        }

                                        [moc save:&error];
                                        [moc refreshObject:stackEntryDatumMO mergeChanges:YES];
                                        stackEntryDatumMO = nil ;
                                        row = 1;
                                    }

                                    [stackEntryDictionary removeAllObjects];

                                    locusId = newLocusId;
                                    stackEntryDatumMO = [NSEntityDescription insertNewObjectForEntityForName:@"StackEntryDatum" inManagedObjectContext:document.managedObjectContext];
                                    stackEntryDatumMO.sampleId = sampleMO.sampleId;
                                    stackEntryDatumMO.name = sampleMO.name;
                                    stackEntryDatumMO.tagId = [NSNumber numberWithInteger:locusId];

                                }

                                @autoreleasepool {
                                    NSString *relationship = [NSString stringWithUTF8String:parts[6].c_str()];

                                    NSDictionary *stackDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                            [NSString stringWithUTF8String:parts[6].c_str()], @"relationship"
                                            , [NSString stringWithUTF8String:parts[8].c_str()], @"sequenceId"
                                            , [NSString stringWithUTF8String:parts[9].c_str()], @"sequence"
                                            , [NSString stringWithUTF8String:parts[7].c_str()], @"block"
                                            , [NSNumber numberWithInteger:row], @"entryId"
                                            , nil ];

                                    if ([relationship isEqualToString:@"consensus"] || [relationship isEqualToString:@"model"]) {
                                        [stackEntryDictionary setObject:stackDictionary forKey:relationship];
                                        row = 1;
                                    }
                                    else {
                                        [stackEntryDictionary setObject:stackDictionary forKey:[NSNumber numberWithInt:row].stringValue];
                                        ++row;
                                    }

                                    stackDictionary = nil ;
                                }


                                ++saveCounter;
                                ++totalSaves;
                            }
                        }

                        ++line_num;
                        ++totalLineNum;
                    }
                }

                stackEntryDictionary = nil ;


                gettimeofday(&time4, NULL);
//    NSLog(@"parse entries lines %ld produce %ld - %ld", fileData.count, datumMO.stackEntries.count, (time4.tv_sec - time3.tv_sec));
                NSLog(@"parse entries lines %ld saves %ld time: %ld s",totalLineNum,totalSaves, (time4.tv_sec - time3.tv_sec));

            }

            [moc save:&error];
            [moc refreshObject:sampleMO mergeChanges:YES];

            fh.close();
            sampleMO = nil ;

        }
        else {
            NSLog(@"not loading tag file %@", tagFileName);
        }
        [progressWindow.loadProgress incrementBy:incrementAmount];
        ++fileNumber;
    }
    gettimeofday(&time2, NULL);

    NSLog(@"STACK ENTRY LOAD: time for %ld  and loci %ld file: %ld s", numFiles, document.loci.count, (time2.tv_sec - time1.tv_sec));

    // save old
    NSError *saveError;
    [document.managedObjectContext save:&saveError];
    NSLog(@"regular save");
    if (saveError != nil ) {
        NSLog(@"error saving %@", saveError);
    }

}

//- (NSDictionary *)getLocusSnpsForDocument:(StacksDocument *)document {
//    if (locusSnpMap == nil) {
//        locusSnpMap = [NSMutableDictionary dictionary];
//        for (LocusMO *locusMO in document.loci) {
//            [locusSnpMap setObject:locusMO.snpData forKey:locusMO.locusId];
//        }
//    }
//    return locusSnpMap;
//}


- (void)addSamplesToDocument:(StacksDocument *)document forSampleIds:(vector<int>)sampleIds andSamples:(map<int, string>)samples {

    if (document.populationLookup == nil || document.populationLookup.count == 0) {
        PopulationMO *populationMO = [[PopulationRepository sharedInstance] insertPopulation:document.managedObjectContext id:[NSNumber numberWithInt:1] name:@"All"];
        document.populations = [NSSet setWithObjects:populationMO, nil];

        // set each sample to populationMO
        for (int i = 0; i < sampleIds.size(); i++) {
            SampleMO *sampleMO = [[SampleRepository sharedInstance] insertSample:document.managedObjectContext id:[NSNumber numberWithInt:sampleIds[i]] name:[NSString stringWithUTF8String:samples[sampleIds[i]].c_str()]];

            [populationMO addSamplesObject:sampleMO];
        }
    }
    else {
        // else
        for (int i = 0; i < sampleIds.size(); i++) {
            SampleMO *sampleMO = [[SampleRepository sharedInstance] insertSample:document.managedObjectContext
                                                                              id:[NSNumber numberWithInt:sampleIds[i]]
                                                                            name:[NSString stringWithUTF8String:samples[sampleIds[i]].c_str()]];

            NSString *populationId = [document.populationLookup objectForKey:sampleMO.name];

            if (populationId != nil) {
                // lets get the population . . can use lookup, but this is usually pretty small
                NSLog(@"tyring to populate popid %@", populationId);
                for (PopulationMO *populationMO in document.populations) {
                    NSNumber *endNumber = [numberFormatter numberFromString:populationId];
                    NSLog(@"comparing to %@ vs %@", populationMO.populationId, endNumber);
                    if ([populationMO.populationId isEqualToNumber:endNumber]) {
                        NSLog(@"FOuND population ID %@", populationMO.populationId);
                        [populationMO addSamplesObject:sampleMO];
                    }
                }
            }
        }
    }

}

- (void)addPopulationsToDocument:(StacksDocument *)document forPath:(NSString *)path {
    // sample . . . index / name
    NSMutableDictionary *populationLookup = [self loadPopulation:path];

    document.populationLookup = populationLookup;

    NSLog(@"population lookup %ld", document.populationLookup.count);


    // scan and create the populations . . .
//    [f setNumberStyle:NSNumberFormatterDecimalStyle];

    // creates a unique set
    NSMutableSet *populationIdsSet = [NSMutableSet set];
    for (NSString *populationId in populationLookup.allValues) {
        [populationIdsSet addObject:populationId];
    }

    for (NSString *popId in populationIdsSet) {
        NSLog(@"adding population %@", popId);
        NSNumber *myNumber = [numberFormatter numberFromString:popId];
        [[PopulationRepository sharedInstance] insertPopulation:document.managedObjectContext id:myNumber name:popId];
    }

    NSArray *populationArray = [[PopulationRepository sharedInstance] getAllPopulations:document.managedObjectContext];

    document.populations = [NSSet setWithArray:populationArray];

}

- (void)readPopulations:(StacksDocument *)document {
    NSMutableArray *populations = [NSMutableArray array];
    NSManagedObjectContext *moc = document.managedObjectContext;
    [moc setUndoManager:nil];

    NSString *path = document.path;
    NSLog(@"reading population for file %@", path);

    NSString *popmapFile = [path stringByAppendingString:@"popmap"];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    BOOL exists = [fileManager fileExistsAtPath:popmapFile];

    if (exists) {
        NSArray *fileData = [[NSString stringWithContentsOfFile:popmapFile encoding:NSUTF8StringEncoding error:nil] componentsSeparatedByString:@"\n"];
        NSString *line;
        for (line in fileData) {
            NSArray *columns = [line componentsSeparatedByString:@"\t"];
            if (columns.count == 2) {

//                [populationLookup setObject:[columns objectAtIndex:1] forKey:[columns objectAtIndex:0]];
//                NSString *sampleName = [columns objectAtIndex:0]; // the sample name . . . male, female, progeny, etc.

                NSString *populationName = [columns objectAtIndex:1]; // initially an integer
                PopulationMO *newPopulationMO = [[PopulationRepository sharedInstance] getPopulation:moc name:populationName];

                if (newPopulationMO != nil) {
                    [populations addObject:newPopulationMO];
                }
            }
            else {
                NSLog(@"something wrong with the column count %@", line);
            }
        }
    }
    else {
        NSLog(@"does not exist at %@", popmapFile);
    }

    NSLog(@"population size %ld", populations.count);
//    [document.populations setWithArray:populations ];
    document.populations = [NSSet setWithArray:populations];
//    return populationLookup;

}

- (StacksDocument *)createStacksDocumentForPath:(NSString *)path {
    NSError *stacksDocumentCreateError;
    StacksDocument *stacksDocument = [[StacksDocument alloc] initWithType:NSSQLiteStoreType error:&stacksDocumentCreateError];
    stacksDocument.path = path;
    stacksDocument.name = path.lastPathComponent;
//    NSManagedObjectContext *context = [[NSManagedObjectContext alloc] initWithConcurrencyType:NSPrivateQueueConcurrencyType];

//    NSManagedObjectContext *moc = [stacksDocument getContextForPath:path];
    NSManagedObjectContext *moc = [self getContextForPath:path andName:path.lastPathComponent andDocument:stacksDocument];
    [moc setUndoManager:nil];
    stacksDocument.managedObjectContext = moc;
    if (stacksDocumentCreateError) {
        NSLog(@"error creating stacks document %@", stacksDocumentCreateError);
        return nil;
    }
    return stacksDocument;
}

- (void)setParentCounts:(NSManagedObjectContext *)managedObjectContext forFile:(NSString *)file loci:(NSSet *)loci {
    NSArray *fileData = [[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] componentsSeparatedByString:@"\n"];

    NSLog(@"creating map %ld", loci.count);
    NSMutableDictionary *lociLookup = [NSMutableDictionary dictionaryWithCapacity:loci.count];
    for (LocusMO *locusMO in loci) {
        [lociLookup setObject:locusMO forKey:locusMO.locusId];
    }
    NSLog(@"processing parents %ld", lociLookup.count);
    NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
    numberFormatter.numberStyle = NSNumberFormatterNoStyle;

    NSString *line;
    LocusMO *locusMO;
    for (line in fileData) {
        NSArray *columns = [line componentsSeparatedByString:@"\t"];
// should be column 8
        if (columns.count > 9) {
            @autoreleasepool {
                NSNumber *locusId = [numberFormatter numberFromString:columns[2]];
                NSArray *parents = [columns[8] componentsSeparatedByString:@","];

                NSInteger parentCount = countParents(parents);
                locusMO = [lociLookup objectForKey:locusId];

                locusMO.parentCount = [NSNumber numberWithInteger:parentCount];
            }
        }
    }
}

- (void)dealloc {
    [sampleLookupDictionary removeAllObjects];
    persistentStoreCoordinator = nil ;
}

//- (StacksDocument *)getStacksDocumentForPath:(NSString *)path {
//    NSError *stacksDocumentCreateError;
//    StacksDocument *stacksDocument = [[StacksDocument alloc] initWithType:NSSQLiteStoreType error:&stacksDocumentCreateError];
//    NSManagedObjectContext *moc = [stacksDocument getContextForPath:path];
////    stacksDocument.managedObjectContext = moc ;
////    stacksDocument.path = path;
//    if (stacksDocumentCreateError) {
//        NSLog(@"error creating stacks document %@", stacksDocumentCreateError);
//        return nil;
//    }
//    return stacksDocument;
//}
@end

NSUInteger countParents(NSArray *parents) {
    NSUInteger parentCount = 0;

    NSMutableSet *parentIds = [NSMutableSet setWithCapacity:parents.count];
    for (NSString *parentString in parents) {
        [parentIds addObject:[parentString componentsSeparatedByString:@"_"][0]];
    }
    parentCount = parentIds.count;

    return parentCount;
}



//void setParentCounts(NSString *file) {
//    NSArray *fileData = [[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] componentsSeparatedByString:@"\n"];
//    NSString *line;
//    for (line in fileData) {
//        NSArray *columns = [line componentsSeparatedByString:@"\t"];
//        // should be column 8
//        if (columns.count > 9) {
//            NSNumber *locusId = [NSNumber numberWithInteger:[[NSString stringWithFormat:@"%@", columns[2]] integerValue]];
////            NSNumber *lookupKey = [NSNumber numberWithInteger:[[NSString stringWithFormat:@"%d", it->first] integerValue]];
//            NSArray *parents = [columns[8] componentsSeparatedByString:@","];
//
//            NSInteger parentCount = countParents(parents);
//            NSLog(@"parent count for %@ is %ld",locusId,parentCount);
//
//            ((LocusMO *) [dictionary objectForKey:locusId]).parentCount = [NSNumber numberWithUnsignedInt:parentCount];
//        }
//    }
//}

//void setParentCounts(NSMutableDictionary *dictionary, NSString *file) {
//    NSArray *fileData = [[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] componentsSeparatedByString:@"\n"];
//    NSString *line;
//    for (line in fileData) {
//        NSArray *columns = [line componentsSeparatedByString:@"\t"];
//        // should be column 8
//        if (columns.count > 9) {
//            NSNumber *locusId = [NSNumber numberWithInteger:[[NSString stringWithFormat:@"%@", columns[2]] integerValue]];
////            NSNumber *lookupKey = [NSNumber numberWithInteger:[[NSString stringWithFormat:@"%d", it->first] integerValue]];
//            NSArray *parents = [columns[8] componentsSeparatedByString:@","];
//
//            NSInteger parentCount = countParents(parents);
//            NSLog(@"parent count for %@ is %ld",locusId,parentCount);
//            ((LocusMO *) [dictionary objectForKey:locusId]).parentCount = [NSNumber numberWithUnsignedInt:parentCount];
//        }
//    }
//}


NSString *calculateType(NSString *file) {
    NSArray *fileData = [[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] componentsSeparatedByString:@"\n"];
    NSString *line;
    for (line in fileData) {
        NSArray *columns = [line componentsSeparatedByString:@"\t"];
        // should be column 8
//        NSLog(@"count is %ld",columns.count);
        if (columns.count > 9) {
            NSArray *parents = [columns[8] componentsSeparatedByString:@","];
            NSInteger parentCount = countParents(parents);
//            NSLog(@"parents %@ count %ld",columns[8],parentCount);
            if (parentCount > 2) {
                return @"Population";
            }
        }
    }
    return @"GeneticMap";
}


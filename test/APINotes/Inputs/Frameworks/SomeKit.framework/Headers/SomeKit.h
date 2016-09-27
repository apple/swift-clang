#ifndef SOMEKIT_H
#define SOMEKIT_H

__attribute__((objc_root_class))
@interface A
-(A*)transform:(A*)input;
-(A*)transform:(A*)input integer:(int)integer;

@property (nonatomic, readonly, retain) A* someA;
@property (nonatomic, retain) A* someOtherA;

@property (nonatomic) int intValue;
@end

@interface B : A
@end

@interface C : A
- (instancetype)init;
- (instancetype)initWithA:(A*)a;
@end

@interface ProcessInfo : A
+(instancetype)processInfo;
@end

@interface A(NonNullProperties)
@property (nonatomic, readwrite, retain) A *nonnullAInstance;
@property (class, nonatomic, readwrite, retain) A *nonnullAInstance;

@property (nonatomic, readwrite, retain) A *nonnullAClass;
@property (class, nonatomic, readwrite, retain) A *nonnullAClass;

@property (nonatomic, readwrite, retain) A *nonnullABoth;
@property (class, nonatomic, readwrite, retain) A *nonnullABoth;
@end

#endif

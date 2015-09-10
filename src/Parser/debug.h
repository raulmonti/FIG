//#define __DEBUG__

#ifdef __DEBUG__
#define __debug__(x) cout << x;
#else
#define __debug__(x) ;
#endif

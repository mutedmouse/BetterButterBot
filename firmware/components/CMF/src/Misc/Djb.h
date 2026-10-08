#ifndef CMF_DJB_H
#define CMF_DJB_H

#include <cinttypes>

#define DJB_SEED 5381

/**
 * @brief A hash function used to calculate a hash of a character given a starting value (seed).
 * @param start Starting value / seed.
 * @param character Subject of hash calculation.
 * @return The DJB hash calculated with the starting seed and given character.
 */
static inline constexpr uint32_t DJB_SH(uint32_t start, uint8_t character) noexcept {
	return (start << 5) + start + character;
}

#define DJB_SH_1(a) DJB_SH(DJB_SEED, (a)[0])
#define DJB_SH_2(a) DJB_SH(DJB_SH_1(a), (a)[1])
#define DJB_SH_3(a) DJB_SH(DJB_SH_2(a), (a)[2])
#define DJB_SH_4(a) DJB_SH(DJB_SH_3(a), (a)[3])
#define DJB_SH_5(a) DJB_SH(DJB_SH_4(a), (a)[4])
#define DJB_SH_6(a) DJB_SH(DJB_SH_5(a), (a)[5])
#define DJB_SH_7(a) DJB_SH(DJB_SH_6(a), (a)[6])
#define DJB_SH_8(a) DJB_SH(DJB_SH_7(a), (a)[7])
#define DJB_SH_9(a) DJB_SH(DJB_SH_8(a), (a)[8])
#define DJB_SH_10(a) DJB_SH(DJB_SH_9(a), (a)[9])
#define DJB_SH_11(a) DJB_SH(DJB_SH_10(a), (a)[10])
#define DJB_SH_12(a) DJB_SH(DJB_SH_11(a), (a)[11])
#define DJB_SH_13(a) DJB_SH(DJB_SH_12(a), (a)[12])
#define DJB_SH_14(a) DJB_SH(DJB_SH_13(a), (a)[13])
#define DJB_SH_15(a) DJB_SH(DJB_SH_14(a), (a)[14])
#define DJB_SH_16(a) DJB_SH(DJB_SH_15(a), (a)[15])
#define DJB_SH_17(a) DJB_SH(DJB_SH_16(a), (a)[16])
#define DJB_SH_18(a) DJB_SH(DJB_SH_17(a), (a)[17])
#define DJB_SH_19(a) DJB_SH(DJB_SH_18(a), (a)[18])
#define DJB_SH_20(a) DJB_SH(DJB_SH_19(a), (a)[19])
#define DJB_SH_21(a) DJB_SH(DJB_SH_20(a), (a)[20])
#define DJB_SH_22(a) DJB_SH(DJB_SH_21(a), (a)[21])
#define DJB_SH_23(a) DJB_SH(DJB_SH_22(a), (a)[22])
#define DJB_SH_24(a) DJB_SH(DJB_SH_23(a), (a)[23])
#define DJB_SH_25(a) DJB_SH(DJB_SH_24(a), (a)[24])
#define DJB_SH_26(a) DJB_SH(DJB_SH_25(a), (a)[25])
#define DJB_SH_27(a) DJB_SH(DJB_SH_26(a), (a)[26])
#define DJB_SH_28(a) DJB_SH(DJB_SH_27(a), (a)[27])
#define DJB_SH_29(a) DJB_SH(DJB_SH_28(a), (a)[28])
#define DJB_SH_30(a) DJB_SH(DJB_SH_29(a), (a)[29])
#define DJB_SH_31(a) DJB_SH(DJB_SH_30(a), (a)[30])
#define DJB_SH_32(a) DJB_SH(DJB_SH_31(a), (a)[31])

/**
 * @brief Macro used to calculate a hash of a C-style string
 * @param a String for hash calculation
 */
#define STRING_HASH(a) ( 										\
    (void)(sizeof(int[(sizeof(a) > 33 ? -1 : 1)])), 			\
    (sizeof(a) == 1) ? DJB_SEED : 								\
    (sizeof(a) == 2) ? DJB_SH_1(a) : 							\
    (sizeof(a) == 3) ? DJB_SH_2(a) : 							\
    (sizeof(a) == 4) ? DJB_SH_3(a) : 							\
    (sizeof(a) == 4) ? DJB_SH_3(a) : 							\
    (sizeof(a) == 5) ? DJB_SH_4(a) : 							\
    (sizeof(a) == 6) ? DJB_SH_5(a) : 							\
    (sizeof(a) == 7) ? DJB_SH_6(a) : 							\
    (sizeof(a) == 8) ? DJB_SH_7(a) : 							\
    (sizeof(a) == 9) ? DJB_SH_8(a) : 							\
    (sizeof(a) == 10) ? DJB_SH_9(a) : 							\
    (sizeof(a) == 11) ? DJB_SH_10(a) : 							\
    (sizeof(a) == 12) ? DJB_SH_11(a) : 							\
    (sizeof(a) == 13) ? DJB_SH_12(a) : 							\
    (sizeof(a) == 14) ? DJB_SH_13(a) : 							\
    (sizeof(a) == 15) ? DJB_SH_14(a) : 							\
    (sizeof(a) == 16) ? DJB_SH_15(a) : 							\
    (sizeof(a) == 17) ? DJB_SH_16(a) : 							\
    (sizeof(a) == 18) ? DJB_SH_17(a) : 							\
    (sizeof(a) == 19) ? DJB_SH_18(a) : 							\
    (sizeof(a) == 20) ? DJB_SH_19(a) : 							\
    (sizeof(a) == 21) ? DJB_SH_20(a) : 							\
    (sizeof(a) == 22) ? DJB_SH_21(a) : 							\
    (sizeof(a) == 23) ? DJB_SH_22(a) : 							\
    (sizeof(a) == 24) ? DJB_SH_23(a) : 							\
    (sizeof(a) == 25) ? DJB_SH_24(a) : 							\
    (sizeof(a) == 26) ? DJB_SH_25(a) : 							\
    (sizeof(a) == 27) ? DJB_SH_26(a) : 							\
    (sizeof(a) == 28) ? DJB_SH_27(a) : 							\
    (sizeof(a) == 29) ? DJB_SH_28(a) : 							\
    (sizeof(a) == 30) ? DJB_SH_29(a) : 							\
    (sizeof(a) == 31) ? DJB_SH_30(a) : 							\
    (sizeof(a) == 32) ? DJB_SH_31(a) : 							\
    (sizeof(a) == 33) ? DJB_SH_32(a) : 							\
    0)															\

/**
 * @brief Macro used to calculate a hash of a C++ STD string
 * @param a String for hash calculation
 */
#define STD_STRING_HASH(a) ( 								    \
    (void)(sizeof(int[(a.size() > 33 ? -1 : 1)])), 			    \
    (a.size() == 1) ? DJB_SEED : 								\
    (a.size() == 2) ? DJB_SH_1(a) : 							\
    (a.size() == 3) ? DJB_SH_2(a) : 							\
    (a.size() == 4) ? DJB_SH_3(a) : 							\
    (a.size() == 4) ? DJB_SH_3(a) : 							\
    (a.size() == 5) ? DJB_SH_4(a) : 							\
    (a.size() == 6) ? DJB_SH_5(a) : 							\
    (a.size() == 7) ? DJB_SH_6(a) : 							\
    (a.size() == 8) ? DJB_SH_7(a) : 							\
    (a.size() == 9) ? DJB_SH_8(a) : 							\
    (a.size() == 10) ? DJB_SH_9(a) : 							\
    (a.size() == 11) ? DJB_SH_10(a) : 							\
    (a.size() == 12) ? DJB_SH_11(a) : 							\
    (a.size() == 13) ? DJB_SH_12(a) : 							\
    (a.size() == 14) ? DJB_SH_13(a) : 							\
    (a.size() == 15) ? DJB_SH_14(a) : 							\
    (a.size() == 16) ? DJB_SH_15(a) : 							\
    (a.size() == 17) ? DJB_SH_16(a) : 							\
    (a.size() == 18) ? DJB_SH_17(a) : 							\
    (a.size() == 19) ? DJB_SH_18(a) : 							\
    (a.size() == 20) ? DJB_SH_19(a) : 							\
    (a.size() == 21) ? DJB_SH_20(a) : 							\
    (a.size() == 22) ? DJB_SH_21(a) : 							\
    (a.size() == 23) ? DJB_SH_22(a) : 							\
    (a.size() == 24) ? DJB_SH_23(a) : 							\
    (a.size() == 25) ? DJB_SH_24(a) : 							\
    (a.size() == 26) ? DJB_SH_25(a) : 							\
    (a.size() == 27) ? DJB_SH_26(a) : 							\
    (a.size() == 28) ? DJB_SH_27(a) : 							\
    (a.size() == 29) ? DJB_SH_28(a) : 							\
    (a.size() == 30) ? DJB_SH_29(a) : 							\
    (a.size() == 31) ? DJB_SH_30(a) : 							\
    (a.size() == 32) ? DJB_SH_31(a) : 							\
    0)															\

#endif //CMF_DJB_H

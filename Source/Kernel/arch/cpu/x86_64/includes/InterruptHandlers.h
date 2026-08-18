/*******************************************************************************
 * @file InterruptHandlers.h
 *
 * @see Cpu.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/06/2026
 *
 * @version 1.0
 *
 * @brief X86 64 CPU interrupt handlers declarations.
 *
 * @details X86 64 CPU interrupt handlers declarations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifndef __CPU_X86_64_INTERRUPT_HANDLERS_H_
#define __CPU_X86_64_INTERRUPT_HANDLERS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 * @brief Assembly interrupt handler for line 0.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler0(void);
/**
 * @brief Assembly interrupt handler for line 1.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler1(void);
/**
 * @brief Assembly interrupt handler for line 2.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler2(void);
/**
 * @brief Assembly interrupt handler for line 3.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler3(void);
/**
 * @brief Assembly interrupt handler for line 4.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler4(void);
/**
 * @brief Assembly interrupt handler for line 5.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler5(void);
/**
 * @brief Assembly interrupt handler for line 6.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler6(void);
/**
 * @brief Assembly interrupt handler for line 7.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler7(void);
/**
 * @brief Assembly interrupt handler for line 8.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler8(void);
/**
 * @brief Assembly interrupt handler for line 9.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler9(void);
/**
 * @brief Assembly interrupt handler for line 10.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler10(void);
/**
 * @brief Assembly interrupt handler for line 11.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler11(void);
/**
 * @brief Assembly interrupt handler for line 12.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler12(void);
/**
 * @brief Assembly interrupt handler for line 13.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler13(void);
/**
 * @brief Assembly interrupt handler for line 14.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler14(void);
/**
 * @brief Assembly interrupt handler for line 15.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler15(void);
/**
 * @brief Assembly interrupt handler for line 16.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler16(void);
/**
 * @brief Assembly interrupt handler for line 17.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler17(void);
/**
 * @brief Assembly interrupt handler for line 18.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler18(void);
/**
 * @brief Assembly interrupt handler for line 19.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler19(void);
/**
 * @brief Assembly interrupt handler for line 20.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler20(void);
/**
 * @brief Assembly interrupt handler for line 21.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler21(void);
/**
 * @brief Assembly interrupt handler for line 22.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler22(void);
/**
 * @brief Assembly interrupt handler for line 23.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler23(void);
/**
 * @brief Assembly interrupt handler for line 24.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler24(void);
/**
 * @brief Assembly interrupt handler for line 25.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler25(void);
/**
 * @brief Assembly interrupt handler for line 26.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler26(void);
/**
 * @brief Assembly interrupt handler for line 27.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler27(void);
/**
 * @brief Assembly interrupt handler for line 28.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler28(void);
/**
 * @brief Assembly interrupt handler for line 29.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler29(void);
/**
 * @brief Assembly interrupt handler for line 30.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler30(void);
/**
 * @brief Assembly interrupt handler for line 31.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler31(void);
/**
 * @brief Assembly interrupt handler for line 32.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler32(void);
/**
 * @brief Assembly interrupt handler for line 33.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler33(void);
/**
 * @brief Assembly interrupt handler for line 34.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler34(void);
/**
 * @brief Assembly interrupt handler for line 35.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler35(void);
/**
 * @brief Assembly interrupt handler for line 36.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler36(void);
/**
 * @brief Assembly interrupt handler for line 37.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler37(void);
/**
 * @brief Assembly interrupt handler for line 38.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler38(void);
/**
 * @brief Assembly interrupt handler for line 39.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler39(void);
/**
 * @brief Assembly interrupt handler for line 40.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler40(void);
/**
 * @brief Assembly interrupt handler for line 41.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler41(void);
/**
 * @brief Assembly interrupt handler for line 42.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler42(void);
/**
 * @brief Assembly interrupt handler for line 43.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler43(void);
/**
 * @brief Assembly interrupt handler for line 44.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler44(void);
/**
 * @brief Assembly interrupt handler for line 45.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler45(void);
/**
 * @brief Assembly interrupt handler for line 46.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler46(void);
/**
 * @brief Assembly interrupt handler for line 47.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler47(void);
/**
 * @brief Assembly interrupt handler for line 48.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler48(void);
/**
 * @brief Assembly interrupt handler for line 49.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler49(void);
/**
 * @brief Assembly interrupt handler for line 50.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler50(void);
/**
 * @brief Assembly interrupt handler for line 51.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler51(void);
/**
 * @brief Assembly interrupt handler for line 52.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler52(void);
/**
 * @brief Assembly interrupt handler for line 53.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler53(void);
/**
 * @brief Assembly interrupt handler for line 54.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler54(void);
/**
 * @brief Assembly interrupt handler for line 55.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler55(void);
/**
 * @brief Assembly interrupt handler for line 56.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler56(void);
/**
 * @brief Assembly interrupt handler for line 57.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler57(void);
/**
 * @brief Assembly interrupt handler for line 58.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler58(void);
/**
 * @brief Assembly interrupt handler for line 59.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler59(void);
/**
 * @brief Assembly interrupt handler for line 60.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler60(void);
/**
 * @brief Assembly interrupt handler for line 61.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler61(void);
/**
 * @brief Assembly interrupt handler for line 62.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler62(void);
/**
 * @brief Assembly interrupt handler for line 63.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler63(void);
/**
 * @brief Assembly interrupt handler for line 64.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler64(void);
/**
 * @brief Assembly interrupt handler for line 65.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler65(void);
/**
 * @brief Assembly interrupt handler for line 66.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler66(void);
/**
 * @brief Assembly interrupt handler for line 67.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler67(void);
/**
 * @brief Assembly interrupt handler for line 68.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler68(void);
/**
 * @brief Assembly interrupt handler for line 69.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler69(void);
/**
 * @brief Assembly interrupt handler for line 70.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler70(void);
/**
 * @brief Assembly interrupt handler for line 71.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler71(void);
/**
 * @brief Assembly interrupt handler for line 72.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler72(void);
/**
 * @brief Assembly interrupt handler for line 73.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler73(void);
/**
 * @brief Assembly interrupt handler for line 74.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler74(void);
/**
 * @brief Assembly interrupt handler for line 75.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler75(void);
/**
 * @brief Assembly interrupt handler for line 76.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler76(void);
/**
 * @brief Assembly interrupt handler for line 77.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler77(void);
/**
 * @brief Assembly interrupt handler for line 78.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler78(void);
/**
 * @brief Assembly interrupt handler for line 79.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler79(void);
/**
 * @brief Assembly interrupt handler for line 80.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler80(void);
/**
 * @brief Assembly interrupt handler for line 81.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler81(void);
/**
 * @brief Assembly interrupt handler for line 82.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler82(void);
/**
 * @brief Assembly interrupt handler for line 83.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler83(void);
/**
 * @brief Assembly interrupt handler for line 84.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler84(void);
/**
 * @brief Assembly interrupt handler for line 85.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler85(void);
/**
 * @brief Assembly interrupt handler for line 86.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler86(void);
/**
 * @brief Assembly interrupt handler for line 87.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler87(void);
/**
 * @brief Assembly interrupt handler for line 88.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler88(void);
/**
 * @brief Assembly interrupt handler for line 89.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler89(void);
/**
 * @brief Assembly interrupt handler for line 90.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler90(void);
/**
 * @brief Assembly interrupt handler for line 91.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler91(void);
/**
 * @brief Assembly interrupt handler for line 92.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler92(void);
/**
 * @brief Assembly interrupt handler for line 93.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler93(void);
/**
 * @brief Assembly interrupt handler for line 94.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler94(void);
/**
 * @brief Assembly interrupt handler for line 95.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler95(void);
/**
 * @brief Assembly interrupt handler for line 96.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler96(void);
/**
 * @brief Assembly interrupt handler for line 97.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler97(void);
/**
 * @brief Assembly interrupt handler for line 98.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler98(void);
/**
 * @brief Assembly interrupt handler for line 99.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler99(void);
/**
 * @brief Assembly interrupt handler for line 100.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler100(void);
/**
 * @brief Assembly interrupt handler for line 101.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler101(void);
/**
 * @brief Assembly interrupt handler for line 102.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler102(void);
/**
 * @brief Assembly interrupt handler for line 103.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler103(void);
/**
 * @brief Assembly interrupt handler for line 104.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler104(void);
/**
 * @brief Assembly interrupt handler for line 105.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler105(void);
/**
 * @brief Assembly interrupt handler for line 106.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler106(void);
/**
 * @brief Assembly interrupt handler for line 107.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler107(void);
/**
 * @brief Assembly interrupt handler for line 108.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler108(void);
/**
 * @brief Assembly interrupt handler for line 109.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler109(void);
/**
 * @brief Assembly interrupt handler for line 110.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler110(void);
/**
 * @brief Assembly interrupt handler for line 111.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler111(void);
/**
 * @brief Assembly interrupt handler for line 112.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler112(void);
/**
 * @brief Assembly interrupt handler for line 113.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler113(void);
/**
 * @brief Assembly interrupt handler for line 114.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler114(void);
/**
 * @brief Assembly interrupt handler for line 115.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler115(void);
/**
 * @brief Assembly interrupt handler for line 116.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler116(void);
/**
 * @brief Assembly interrupt handler for line 117.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler117(void);
/**
 * @brief Assembly interrupt handler for line 118.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler118(void);
/**
 * @brief Assembly interrupt handler for line 119.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler119(void);
/**
 * @brief Assembly interrupt handler for line 120.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler120(void);
/**
 * @brief Assembly interrupt handler for line 121.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler121(void);
/**
 * @brief Assembly interrupt handler for line 122.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler122(void);
/**
 * @brief Assembly interrupt handler for line 123.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler123(void);
/**
 * @brief Assembly interrupt handler for line 124.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler124(void);
/**
 * @brief Assembly interrupt handler for line 125.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler125(void);
/**
 * @brief Assembly interrupt handler for line 126.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler126(void);
/**
 * @brief Assembly interrupt handler for line 127.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler127(void);
/**
 * @brief Assembly interrupt handler for line 128.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler128(void);
/**
 * @brief Assembly interrupt handler for line 129.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler129(void);
/**
 * @brief Assembly interrupt handler for line 130.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler130(void);
/**
 * @brief Assembly interrupt handler for line 131.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler131(void);
/**
 * @brief Assembly interrupt handler for line 132.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler132(void);
/**
 * @brief Assembly interrupt handler for line 133.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler133(void);
/**
 * @brief Assembly interrupt handler for line 134.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler134(void);
/**
 * @brief Assembly interrupt handler for line 135.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler135(void);
/**
 * @brief Assembly interrupt handler for line 136.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler136(void);
/**
 * @brief Assembly interrupt handler for line 137.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler137(void);
/**
 * @brief Assembly interrupt handler for line 138.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler138(void);
/**
 * @brief Assembly interrupt handler for line 139.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler139(void);
/**
 * @brief Assembly interrupt handler for line 140.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler140(void);
/**
 * @brief Assembly interrupt handler for line 141.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler141(void);
/**
 * @brief Assembly interrupt handler for line 142.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler142(void);
/**
 * @brief Assembly interrupt handler for line 143.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler143(void);
/**
 * @brief Assembly interrupt handler for line 144.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler144(void);
/**
 * @brief Assembly interrupt handler for line 145.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler145(void);
/**
 * @brief Assembly interrupt handler for line 146.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler146(void);
/**
 * @brief Assembly interrupt handler for line 147.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler147(void);
/**
 * @brief Assembly interrupt handler for line 148.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler148(void);
/**
 * @brief Assembly interrupt handler for line 149.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler149(void);
/**
 * @brief Assembly interrupt handler for line 150.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler150(void);
/**
 * @brief Assembly interrupt handler for line 151.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler151(void);
/**
 * @brief Assembly interrupt handler for line 152.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler152(void);
/**
 * @brief Assembly interrupt handler for line 153.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler153(void);
/**
 * @brief Assembly interrupt handler for line 154.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler154(void);
/**
 * @brief Assembly interrupt handler for line 155.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler155(void);
/**
 * @brief Assembly interrupt handler for line 156.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler156(void);
/**
 * @brief Assembly interrupt handler for line 157.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler157(void);
/**
 * @brief Assembly interrupt handler for line 158.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler158(void);
/**
 * @brief Assembly interrupt handler for line 159.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler159(void);
/**
 * @brief Assembly interrupt handler for line 160.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler160(void);
/**
 * @brief Assembly interrupt handler for line 161.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler161(void);
/**
 * @brief Assembly interrupt handler for line 162.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler162(void);
/**
 * @brief Assembly interrupt handler for line 163.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler163(void);
/**
 * @brief Assembly interrupt handler for line 164.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler164(void);
/**
 * @brief Assembly interrupt handler for line 165.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler165(void);
/**
 * @brief Assembly interrupt handler for line 166.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler166(void);
/**
 * @brief Assembly interrupt handler for line 167.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler167(void);
/**
 * @brief Assembly interrupt handler for line 168.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler168(void);
/**
 * @brief Assembly interrupt handler for line 169.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler169(void);
/**
 * @brief Assembly interrupt handler for line 170.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler170(void);
/**
 * @brief Assembly interrupt handler for line 171.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler171(void);
/**
 * @brief Assembly interrupt handler for line 172.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler172(void);
/**
 * @brief Assembly interrupt handler for line 173.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler173(void);
/**
 * @brief Assembly interrupt handler for line 174.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler174(void);
/**
 * @brief Assembly interrupt handler for line 175.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler175(void);
/**
 * @brief Assembly interrupt handler for line 176.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler176(void);
/**
 * @brief Assembly interrupt handler for line 177.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler177(void);
/**
 * @brief Assembly interrupt handler for line 178.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler178(void);
/**
 * @brief Assembly interrupt handler for line 179.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler179(void);
/**
 * @brief Assembly interrupt handler for line 180.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler180(void);
/**
 * @brief Assembly interrupt handler for line 181.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler181(void);
/**
 * @brief Assembly interrupt handler for line 182.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler182(void);
/**
 * @brief Assembly interrupt handler for line 183.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler183(void);
/**
 * @brief Assembly interrupt handler for line 184.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler184(void);
/**
 * @brief Assembly interrupt handler for line 185.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler185(void);
/**
 * @brief Assembly interrupt handler for line 186.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler186(void);
/**
 * @brief Assembly interrupt handler for line 187.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler187(void);
/**
 * @brief Assembly interrupt handler for line 188.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler188(void);
/**
 * @brief Assembly interrupt handler for line 189.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler189(void);
/**
 * @brief Assembly interrupt handler for line 190.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler190(void);
/**
 * @brief Assembly interrupt handler for line 191.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler191(void);
/**
 * @brief Assembly interrupt handler for line 192.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler192(void);
/**
 * @brief Assembly interrupt handler for line 193.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler193(void);
/**
 * @brief Assembly interrupt handler for line 194.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler194(void);
/**
 * @brief Assembly interrupt handler for line 195.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler195(void);
/**
 * @brief Assembly interrupt handler for line 196.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler196(void);
/**
 * @brief Assembly interrupt handler for line 197.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler197(void);
/**
 * @brief Assembly interrupt handler for line 198.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler198(void);
/**
 * @brief Assembly interrupt handler for line 199.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler199(void);
/**
 * @brief Assembly interrupt handler for line 200.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler200(void);
/**
 * @brief Assembly interrupt handler for line 201.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler201(void);
/**
 * @brief Assembly interrupt handler for line 202.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler202(void);
/**
 * @brief Assembly interrupt handler for line 203.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler203(void);
/**
 * @brief Assembly interrupt handler for line 204.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler204(void);
/**
 * @brief Assembly interrupt handler for line 205.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler205(void);
/**
 * @brief Assembly interrupt handler for line 206.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler206(void);
/**
 * @brief Assembly interrupt handler for line 207.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler207(void);
/**
 * @brief Assembly interrupt handler for line 208.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler208(void);
/**
 * @brief Assembly interrupt handler for line 209.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler209(void);
/**
 * @brief Assembly interrupt handler for line 210.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler210(void);
/**
 * @brief Assembly interrupt handler for line 211.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler211(void);
/**
 * @brief Assembly interrupt handler for line 212.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler212(void);
/**
 * @brief Assembly interrupt handler for line 213.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler213(void);
/**
 * @brief Assembly interrupt handler for line 214.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler214(void);
/**
 * @brief Assembly interrupt handler for line 215.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler215(void);
/**
 * @brief Assembly interrupt handler for line 216.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler216(void);
/**
 * @brief Assembly interrupt handler for line 217.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler217(void);
/**
 * @brief Assembly interrupt handler for line 218.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler218(void);
/**
 * @brief Assembly interrupt handler for line 219.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler219(void);
/**
 * @brief Assembly interrupt handler for line 220.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler220(void);
/**
 * @brief Assembly interrupt handler for line 221.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler221(void);
/**
 * @brief Assembly interrupt handler for line 222.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler222(void);
/**
 * @brief Assembly interrupt handler for line 223.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler223(void);
/**
 * @brief Assembly interrupt handler for line 224.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler224(void);
/**
 * @brief Assembly interrupt handler for line 225.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler225(void);
/**
 * @brief Assembly interrupt handler for line 226.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler226(void);
/**
 * @brief Assembly interrupt handler for line 227.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler227(void);
/**
 * @brief Assembly interrupt handler for line 228.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler228(void);
/**
 * @brief Assembly interrupt handler for line 229.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler229(void);
/**
 * @brief Assembly interrupt handler for line 230.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler230(void);
/**
 * @brief Assembly interrupt handler for line 231.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler231(void);
/**
 * @brief Assembly interrupt handler for line 232.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler232(void);
/**
 * @brief Assembly interrupt handler for line 233.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler233(void);
/**
 * @brief Assembly interrupt handler for line 234.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler234(void);
/**
 * @brief Assembly interrupt handler for line 235.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler235(void);
/**
 * @brief Assembly interrupt handler for line 236.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler236(void);
/**
 * @brief Assembly interrupt handler for line 237.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler237(void);
/**
 * @brief Assembly interrupt handler for line 238.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler238(void);
/**
 * @brief Assembly interrupt handler for line 239.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler239(void);
/**
 * @brief Assembly interrupt handler for line 240.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler240(void);
/**
 * @brief Assembly interrupt handler for line 241.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler241(void);
/**
 * @brief Assembly interrupt handler for line 242.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler242(void);
/**
 * @brief Assembly interrupt handler for line 243.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler243(void);
/**
 * @brief Assembly interrupt handler for line 244.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler244(void);
/**
 * @brief Assembly interrupt handler for line 245.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler245(void);
/**
 * @brief Assembly interrupt handler for line 246.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler246(void);
/**
 * @brief Assembly interrupt handler for line 247.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler247(void);
/**
 * @brief Assembly interrupt handler for line 248.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler248(void);
/**
 * @brief Assembly interrupt handler for line 249.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler249(void);
/**
 * @brief Assembly interrupt handler for line 250.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler250(void);
/**
 * @brief Assembly interrupt handler for line 251.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler251(void);
/**
 * @brief Assembly interrupt handler for line 252.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler252(void);
/**
 * @brief Assembly interrupt handler for line 253.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler253(void);
/**
 * @brief Assembly interrupt handler for line 254.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler254(void);
/**
 * @brief Assembly interrupt handler for line 255.
 * Saves the context and calls the generic interrupt handler
 */
void IntHandler255(void);

#endif /* #ifndef __CPU_X86_64_INTERRUPT_HANDLERS_H_ */

/************************************ EOF *************************************/
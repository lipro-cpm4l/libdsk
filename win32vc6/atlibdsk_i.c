/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Dec 16 09:56:36 2015
 */
/* Compiler settings for J:\C\LibDsk\win32vc6\atlibdsk.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IGeometry = {0x343FE9B1,0x6FFE,0x4708,{0x9A,0x83,0x22,0x7D,0x72,0x03,0x19,0x3C}};


const IID IID_IDisk = {0x2F7EE90E,0x646D,0x4AB0,{0x8B,0xA5,0xCC,0x13,0xDD,0xD4,0x3C,0x39}};


const IID IID_IReporter = {0x3DED12BB,0xABA5,0x48C3,{0x98,0x9A,0x9D,0x6A,0xA8,0xED,0x54,0x2D}};


const IID IID_ILibrary = {0x84FC5CD6,0x91AB,0x42B3,{0xBC,0xEC,0xF9,0x9B,0xCC,0x20,0xED,0xF0}};


const IID LIBID_LIBDSK = {0xF81955B8,0x9ECB,0x4B29,{0x8A,0xCA,0xB7,0x93,0xE2,0xA9,0xDD,0x8E}};


const CLSID CLSID_Library = {0x6AAA65C3,0x2CEA,0x4F6B,{0xAB,0x2B,0xFB,0xDE,0x9D,0x2D,0x48,0x4E}};


const CLSID CLSID_Disk = {0x6A6B3263,0x176B,0x4D21,{0x81,0xA9,0x8F,0x5E,0xEF,0x73,0x9F,0xA9}};


const CLSID CLSID_Geometry = {0x1AFF2A50,0xEEE7,0x4233,{0x83,0xEB,0xEC,0x1C,0x9E,0xBA,0xFA,0x74}};


const CLSID CLSID_Reporter = {0xF011746E,0x3431,0x4C26,{0x85,0x8A,0xD0,0x4D,0x14,0x54,0x63,0x7C}};


#ifdef __cplusplus
}
#endif


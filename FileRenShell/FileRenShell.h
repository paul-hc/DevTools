

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Fri Jan 05 11:00:46 2018
 */
/* Compiler settings for .\FileRenShell.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __FileRenShell_h__
#define __FileRenShell_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFileRenameShell_FWD_DEFINED__
#define __IFileRenameShell_FWD_DEFINED__
typedef interface IFileRenameShell IFileRenameShell;
#endif 	/* __IFileRenameShell_FWD_DEFINED__ */


#ifndef __FileRenameShell_FWD_DEFINED__
#define __FileRenameShell_FWD_DEFINED__

#ifdef __cplusplus
typedef class FileRenameShell FileRenameShell;
#else
typedef struct FileRenameShell FileRenameShell;
#endif /* __cplusplus */

#endif 	/* __FileRenameShell_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IFileRenameShell_INTERFACE_DEFINED__
#define __IFileRenameShell_INTERFACE_DEFINED__

/* interface IFileRenameShell */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFileRenameShell;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D4EA502-89A1-11D5-A57D-0050BA0E2E4A")
    IFileRenameShell : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IFileRenameShellVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFileRenameShell * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFileRenameShell * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFileRenameShell * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFileRenameShell * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFileRenameShell * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFileRenameShell * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFileRenameShell * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IFileRenameShellVtbl;

    interface IFileRenameShell
    {
        CONST_VTBL struct IFileRenameShellVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileRenameShell_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFileRenameShell_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFileRenameShell_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFileRenameShell_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IFileRenameShell_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IFileRenameShell_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IFileRenameShell_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFileRenameShell_INTERFACE_DEFINED__ */



#ifndef __FILERENSHELLLib_LIBRARY_DEFINED__
#define __FILERENSHELLLib_LIBRARY_DEFINED__

/* library FILERENSHELLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_FILERENSHELLLib;

EXTERN_C const CLSID CLSID_FileRenameShell;

#ifdef __cplusplus

class DECLSPEC_UUID("1D4EA504-89A1-11D5-A57D-0050BA0E2E4A")
FileRenameShell;
#endif
#endif /* __FILERENSHELLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



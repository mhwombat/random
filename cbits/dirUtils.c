/* 
 * (c) The University of Glasgow 2002
 *
 * Directory Runtime Support
 */

#include "config.h"

// The following is required on Solaris to force the POSIX versions of
// the various _r functions instead of the Solaris versions.
#ifdef solaris2_TARGET_OS
#define _POSIX_PTHREAD_SEMANTICS
#endif

#include "HsBase.h"

#if defined(mingw32_TARGET_OS)
#include <windows.h>
#endif

/*
 * read an entry from the directory stream; opt for the
 * re-entrant friendly way of doing this, if available.
 */
HsInt
__hscore_readdir( HsAddr dirPtr, HsAddr pDirEnt )
{
  struct dirent **pDirE = (struct dirent**)pDirEnt;
#if HAVE_READDIR_R
  struct dirent* p;
  int res;
  static unsigned int nm_max = -1;
  
  if (pDirE == NULL) {
    return -1;
  }
  if (nm_max == (unsigned int)-1) {
#ifdef NAME_MAX
    nm_max = NAME_MAX + 1;
#else
    nm_max = pathconf(".", _PC_NAME_MAX);
    if (nm_max == -1) { nm_max = 255; }
    nm_max++;
#endif
  }
  p = (struct dirent*)malloc(sizeof(struct dirent) + nm_max);
  if (p == NULL) return -1;
  res = readdir_r((DIR*)dirPtr, p, pDirE);
  if (res != 0) {
    *pDirE = NULL;
    free(p);
  }
  return res;
#else

  if (pDirE == NULL) {
    return -1;
  }

  *pDirE = readdir((DIR*)dirPtr);
  if (*pDirE == NULL) {
    return -1;
  } else {
    return 0;
  }  
#endif
}

/*
 * Function: __hscore_renameFile()
 *
 * Provide Haskell98's semantics for renaming files and directories.
 * It mirrors that of POSIX.1's behaviour for rename() by overwriting
 * the target if it exists (the MS CRT implementation of rename() returns
 * an error
 *
 */
HsInt
__hscore_renameFile( HsAddr src,
		     HsAddr dest)
{
#if defined(_MSC_VER) || defined(_WIN32)
    static int forNT = -1;
    DWORD rc;
    
    /* ToDo: propagate error codes back */
    if (MoveFileA(src, dest)) {
	return 0;
    } else {
	rc = GetLastError();
    }
    
    /* Failed...it could be because the target already existed. */
    if ( !GetFileAttributes(dest) ) {
	/* No, it's not there - just fail. */
	errno = 0;
	return (-1);
    }

    if (forNT == -1) {
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	if ( !GetVersionEx(&ovi) ) {
	    errno = 0; 
	    return (-1);
	}
	forNT = ((ovi.dwPlatformId & VER_PLATFORM_WIN32_NT) != 0);
    }
    
    if (forNT) {
	/* Easy, go for MoveFileEx() */
	if ( MoveFileExA(src, dest, MOVEFILE_REPLACE_EXISTING) ) {
	    return 0;
	} else {
	    errno = 0; 
	    return (-1);
	}
    }

    /* No MoveFileEx() for Win9x, try deleting the target. */
    /* Similarly, if the MoveFile*() ops didn't work out under NT */
    if (DeleteFileA(dest)) {
	if (MoveFileA(src,dest)) {
	    return 0;
	} else {
	    errno = 0;
	    return (-1);
	}
    } else {
	errno = 0;
	return (-1);
    }
#else
    return rename(src,dest);
#endif
}


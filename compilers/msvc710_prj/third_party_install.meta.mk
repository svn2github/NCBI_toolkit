# $Id$

#################################################################
#
# Third party DLLs installation makefile
#
# Author: Andrei Gourianov
#
#################################################################


# Build configuration name
INTDIR = $(INTDIR:.\=)
ALTDIR = $(INTDIR:VTune_=)


# Install into this folder
DESTINATION_ROOT   = .\bin
DESTINATION        = $(DESTINATION_ROOT)\$(INTDIR)

# Extensions of files to copy
EXTENSIONS         = dll pdb

# MSVC DLLs
#  MSVC_SRC must be defined elsewhere (eg, in command line)
#MSVCRT_SRC = \\snowman\win-coremake\Lib\ThirdParty\msvc\msvc71\7.1\bin
#MSVCRT_SRC = \\snowman\win-coremake\Lib\ThirdParty\msvc\msvc8\8\bin

MSVCRT_SRC = \\snowman\win-coremake\Lib\ThirdParty\msvc\$(MSVC_SRC)\bin


#################################################################
# Source folders
#
# x_BINPATH macros are defined in Makefile.third_party.mk 
# generated by project_tree_builder
#

FLTK_SRC = $(FLTK_BINPATH)\$(INTDIR)
!IF !EXIST($(FLTK_SRC))
FLTK_SRC = $(FLTK_BINPATH)\$(ALTDIR)
!ENDIF

BERKELEYDB_SRC = $(BERKELEYDB_BINPATH)\$(INTDIR)
!IF !EXIST($(BERKELEYDB_SRC))
BERKELEYDB_SRC = $(BERKELEYDB_BINPATH)\$(ALTDIR)
!ENDIF

SQLITE_SRC = $(SQLITE_BINPATH)\$(INTDIR)
!IF !EXIST($(SQLITE_SRC))
SQLITE_SRC = $(SQLITE_BINPATH)\$(ALTDIR)
!ENDIF

SQLITE3_SRC = $(SQLITE3_BINPATH)\$(INTDIR)
!IF !EXIST($(SQLITE3_SRC))
SQLITE3_SRC = $(SQLITE3_BINPATH)\$(ALTDIR)
!ENDIF

WXWINDOWS_SRC = $(WXWINDOWS_BINPATH)\$(INTDIR)
!IF !EXIST($(WXWINDOWS_SRC))
WXWINDOWS_SRC = $(WXWINDOWS_BINPATH)\$(ALTDIR)
!ENDIF

WXWIDGETS_SRC = $(WXWIDGETS_BINPATH)\$(INTDIR)
!IF !EXIST($(WXWIDGETS_SRC))
WXWIDGETS_SRC = $(WXWIDGETS_BINPATH)\$(ALTDIR)
!ENDIF

SYBASE_SRC = $(SYBASE_BINPATH)\$(INTDIR)
!IF !EXIST($(SYBASE_SRC))
SYBASE_SRC = $(SYBASE_BINPATH)\$(ALTDIR)
!ENDIF

MYSQL_SRC = $(MYSQL_BINPATH)\$(INTDIR)
!IF !EXIST($(MYSQL_SRC))
MYSQL_SRC = $(MYSQL_BINPATH)\$(ALTDIR)
!ENDIF

MSSQL_SRC = $(MSSQL_BINPATH)\$(INTDIR)
!IF !EXIST($(MSSQL_SRC))
MSSQL_SRC = $(MSSQL_BINPATH)\$(ALTDIR)
!ENDIF

OPENSSL_SRC = $(OPENSSL_BINPATH)\$(INTDIR)
!IF !EXIST($(OPENSSL_SRC))
OPENSSL_SRC = $(OPENSSL_BINPATH)\$(ALTDIR)
!ENDIF

LZO_SRC = $(LZO_BINPATH)\$(INTDIR)
!IF !EXIST($(LZO_SRC))
LZO_SRC = $(LZO_BINPATH)\$(ALTDIR)
!ENDIF

#################################################################
# What to install

INSTALL_LIBS = \
	$(FLTK_SRC).install \
	$(BERKELEYDB_SRC).install \
	$(SQLITE_SRC).install \
	$(SQLITE3_SRC).install \
	$(WXWINDOWS_SRC).install \
	$(WXWIDGETS_SRC).install \
	$(SYBASE_SRC).install \
	$(MYSQL_SRC).install \
	$(MSSQL_SRC).install \
	$(OPENSSL_SRC).install \
	$(LZO_SRC).install


CLEAN_LIBS = \
	$(FLTK_SRC).clean \
	$(BERKELEYDB_SRC).clean \
	$(SQLITE_SRC).clean \
	$(SQLITE3_SRC).clean \
	$(WXWINDOWS_SRC).clean \
	$(WXWIDGETS_SRC).clean \
	$(SYBASE_SRC).clean \
	$(MYSQL_SRC).clean \
	$(MSSQL_SRC).clean \
	$(OPENSSL_SRC).clean \
	$(LZO_SRC).clean

#################################################################

INSTALL_CMD = \
	@if exist "$*" ( for %%e in ($(EXTENSIONS)) do @( \
	    if exist "$*\*.%%e" ( \
	      for /f "delims=" %%i in ('dir /a-d/b "$*\*.%%e"') do @( \
	        xcopy /Y /D /F "$*\%%i" "$(DESTINATION)" ) \
	    ) else (echo WARNING:   "$*\*.%%e" not found) ) \
	) else (echo ERROR:   "$*" not found)

CLEAN_CMD = \
	@if exist "$*" ( for %%e in ($(EXTENSIONS)) do @( \
	    if exist "$*\*.%%e" ( \
	      for /f "delims=" %%i in ('dir /a-d/b "$*\*.%%e"') do @( \
	        if exist "$(DESTINATION)\%%i" ( echo $(DESTINATION)\%%i & del /F "$(DESTINATION)\%%i" )))) \
	) else (echo ERROR:   "$*" not found)


#################################################################
# Targets
#

all : dirs $(INSTALL_LIBS)

clean : $(CLEAN_LIBS)

rebuild : clean all

dirs :
    @if not exist $(DESTINATION) (echo Creating directory $(DESTINATION)... & mkdir $(DESTINATION))



$(FLTK_SRC).install :
	@echo ---- & echo Copying FLTK DLLs & $(INSTALL_CMD)

$(FLTK_SRC).clean :
	@echo ---- & echo Deleting FLTK DLLs & $(CLEAN_CMD)


$(BERKELEYDB_SRC).install :
	@echo ---- & echo Copying BerkeleyDB DLLs & $(INSTALL_CMD)

$(BERKELEYDB_SRC).clean :
	@echo ---- & echo Deleting BerkeleyDB DLLs & $(CLEAN_CMD)


$(SQLITE_SRC).install :
	@echo ---- & echo Copying SQLite DLLs & $(INSTALL_CMD)

$(SQLITE_SRC).clean :
	@echo ---- & echo Deleting SQLite DLLs & $(CLEAN_CMD)


$(SQLITE3_SRC).install :
	@echo ---- & echo Copying SQLite3 DLLs & $(INSTALL_CMD)

$(SQLITE3_SRC).clean :
	@echo ---- & echo Deleting SQLite3 DLLs & $(CLEAN_CMD)


$(WXWINDOWS_SRC).install :
	@echo ---- & echo Copying wxWindows DLLs & $(INSTALL_CMD)

$(WXWINDOWS_SRC).clean :
	@echo ---- & echo Deleting wxWindows DLLs & $(CLEAN_CMD)


$(WXWIDGETS_SRC).install :
	@echo ---- & echo Copying wxWidgets DLLs & $(INSTALL_CMD)

$(WXWIDGETS_SRC).clean :
	@echo ---- & echo Deleting wxWidgets DLLs & $(CLEAN_CMD)


$(SYBASE_SRC).install :
	@echo ---- & echo Copying Sybase DLLs & $(INSTALL_CMD)

$(SYBASE_SRC).clean :
	@echo ---- & echo Deleting Sybase DLLs & $(CLEAN_CMD)


$(MYSQL_SRC).install :
	@echo ---- & echo Copying MySQL DLLs & $(INSTALL_CMD)

$(MYSQL_SRC).clean :
	@echo ---- & echo Deleting MySQL DLLs & $(CLEAN_CMD)


$(MSSQL_SRC).install :
	@echo ---- & echo Copying MSSQL DLLs & $(INSTALL_CMD)

$(MSSQL_SRC).clean :
	@echo ---- & echo Deleting MSSQL DLLs & $(CLEAN_CMD)


$(OPENSSL_SRC).install :
	@echo ---- & echo Copying OpenSSL DLLs & $(INSTALL_CMD)

$(OPENSSL_SRC).clean :
	@echo ---- & echo Deleting OpenSSL DLLs & $(CLEAN_CMD)


$(LZO_SRC).install :
	@echo ---- & echo Copying LZO DLLs & $(INSTALL_CMD)

$(LZO_SRC).clean :
	@echo ---- & echo Deleting LZO DLLs & $(CLEAN_CMD)


# -----------------------------------------
# MSVC RT DLLs are not included into 'all'

msvc_install : dirs $(MSVCRT_SRC).install

msvc_clean : $(MSVCRT_SRC).clean

msvc_rebuild : msvc_clean msvc_install

$(MSVCRT_SRC).install :
	@echo ---- & echo Copying MSVC DLLs & $(INSTALL_CMD)

$(MSVCRT_SRC).clean :
	@echo ---- & echo Deleting MSVC DLLs & $(CLEAN_CMD)

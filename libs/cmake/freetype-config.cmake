add_library(spfreetype2 STATIC
    libs/freetype2/src/autofit/autofit.c
        #libs/freetype2/src/autofit/afblue.c
        #libs/freetype2/src/autofit/afcjk.c
        #libs/freetype2/src/autofit/afdummy.c
        #libs/freetype2/src/autofit/afglobal.c
        #libs/freetype2/src/autofit/afhints.c
        #libs/freetype2/src/autofit/afindic.c
        #libs/freetype2/src/autofit/aflatin.c
        #libs/freetype2/src/autofit/afloader.c
        #libs/freetype2/src/autofit/afmodule.c
        #libs/freetype2/src/autofit/afranges.c
        #libs/freetype2/src/autofit/afshaper.c
    libs/freetype2/src/base/ftbase.c
        #libs/freetype2/src/base/ftadvanc.c
        #libs/freetype2/src/base/ftcalc.c
        #libs/freetype2/src/base/ftcolor.c
        #libs/freetype2/src/base/ftdbgmem.c
        #libs/freetype2/src/base/fterrors.c
        #libs/freetype2/src/base/ftfntfmt.c
        #libs/freetype2/src/base/ftgloadr.c
        #libs/freetype2/src/base/fthash.c
        #libs/freetype2/src/base/ftlcdfil.c
        #libs/freetype2/src/base/ftmac.c
        #libs/freetype2/src/base/ftobjs.c
        #libs/freetype2/src/base/ftoutln.c
        #libs/freetype2/src/base/ftpsprop.c
        #libs/freetype2/src/base/ftrfork.c
        #libs/freetype2/src/base/ftsnames.c
        #libs/freetype2/src/base/ftstream.c
        #libs/freetype2/src/base/fttrigon.c
        #libs/freetype2/src/base/ftutil.c
    libs/freetype2/src/base/ftbbox.c
    libs/freetype2/src/base/ftbdf.c
    libs/freetype2/src/base/ftbitmap.c

    libs/freetype2/src/base/ftcid.c


    libs/freetype2/src/base/ftdebug.c


    libs/freetype2/src/base/ftfstype.c
    libs/freetype2/src/base/ftgasp.c

    libs/freetype2/src/base/ftglyph.c
    libs/freetype2/src/base/ftgxval.c

    libs/freetype2/src/base/ftinit.c


    libs/freetype2/src/base/ftmm.c

    libs/freetype2/src/base/ftotval.c

    libs/freetype2/src/base/ftpatent.c
    libs/freetype2/src/base/ftpfr.c




    libs/freetype2/src/base/ftstroke.c
    libs/freetype2/src/base/ftsynth.c
    libs/freetype2/src/base/ftsystem.c

    libs/freetype2/src/base/fttype1.c

    libs/freetype2/src/base/ftver.rc
    libs/freetype2/src/base/ftwinfnt.c
    libs/freetype2/src/base/md5.c
    libs/freetype2/src/bdf/bdf.c
        #libs/freetype2/src/bdf/bdfdrivr.c
        #libs/freetype2/src/bdf/bdflib.c
    libs/freetype2/src/bzip2/ftbzip2.c
    libs/freetype2/src/cache/ftcache.c
        #libs/freetype2/src/cache/ftcbasic.c
        #libs/freetype2/src/cache/ftccache.c
        #libs/freetype2/src/cache/ftccmap.c
        #libs/freetype2/src/cache/ftcglyph.c
        #libs/freetype2/src/cache/ftcimage.c
        #libs/freetype2/src/cache/ftcmanag.c
        #libs/freetype2/src/cache/ftcmru.c
        #libs/freetype2/src/cache/ftcsbits.c
    libs/freetype2/src/cff/cff.c
        #libs/freetype2/src/cff/cffcmap.c
        #libs/freetype2/src/cff/cffdrivr.c
        #libs/freetype2/src/cff/cffgload.c
        #libs/freetype2/src/cff/cffload.c
        #libs/freetype2/src/cff/cffobjs.c
        #libs/freetype2/src/cff/cffparse.c
    libs/freetype2/src/cid/type1cid.c
        #libs/freetype2/src/cid/cidgload.c
        #libs/freetype2/src/cid/cidload.c
        #libs/freetype2/src/cid/cidobjs.c
        #libs/freetype2/src/cid/cidparse.c
        #libs/freetype2/src/cid/cidriver.c
    #libs/freetype2/src/dlg/dlg.c
    #libs/freetype2/src/dlg/dlgwrap.c
    libs/freetype2/src/gxvalid/gxvalid.c
        #libs/freetype2/src/gxvalid/gxvbsln.c
        #libs/freetype2/src/gxvalid/gxvcommn.c
        #libs/freetype2/src/gxvalid/gxvfeat.c
        #libs/freetype2/src/gxvalid/gxvjust.c
        #libs/freetype2/src/gxvalid/gxvkern.c
        #libs/freetype2/src/gxvalid/gxvlcar.c
        #libs/freetype2/src/gxvalid/gxvmod.c
        #libs/freetype2/src/gxvalid/gxvmort.c
        #libs/freetype2/src/gxvalid/gxvmort0.c
        #libs/freetype2/src/gxvalid/gxvmort1.c
        #libs/freetype2/src/gxvalid/gxvmort2.c
        #libs/freetype2/src/gxvalid/gxvmort4.c
        #libs/freetype2/src/gxvalid/gxvmort5.c
        #libs/freetype2/src/gxvalid/gxvmorx.c
        #libs/freetype2/src/gxvalid/gxvmorx0.c
        #libs/freetype2/src/gxvalid/gxvmorx1.c
        #libs/freetype2/src/gxvalid/gxvmorx2.c
        #libs/freetype2/src/gxvalid/gxvmorx4.c
        #libs/freetype2/src/gxvalid/gxvmorx5.c
        #libs/freetype2/src/gxvalid/gxvopbd.c
        #libs/freetype2/src/gxvalid/gxvprop.c
        #libs/freetype2/src/gxvalid/gxvtrak.c
    #libs/freetype2/src/gxvalid/gxvfgen.c
    libs/freetype2/src/gzip/ftgzip.c
        #libs/freetype2/src/gzip/adler32.c
        #libs/freetype2/src/gzip/infblock.c
        #libs/freetype2/src/gzip/infcodes.c
        #libs/freetype2/src/gzip/inflate.c
        #libs/freetype2/src/gzip/inftrees.c
        #libs/freetype2/src/gzip/infutil.c
        #libs/freetype2/src/gzip/zutil.c
    libs/freetype2/src/lzw/ftlzw.c
        #libs/freetype2/src/lzw/ftzopen.c
    libs/freetype2/src/otvalid/otvalid.c
        #libs/freetype2/src/otvalid/otvbase.c
        #libs/freetype2/src/otvalid/otvcommn.c
        #libs/freetype2/src/otvalid/otvgdef.c
        #libs/freetype2/src/otvalid/otvgpos.c
        #libs/freetype2/src/otvalid/otvgsub.c
        #libs/freetype2/src/otvalid/otvjstf.c
        #libs/freetype2/src/otvalid/otvmath.c
        #libs/freetype2/src/otvalid/otvmod.c
    libs/freetype2/src/pcf/pcf.c
        #libs/freetype2/src/pcf/pcfdrivr.c
        #libs/freetype2/src/pcf/pcfread.c
        #libs/freetype2/src/pcf/pcfutil.c
    libs/freetype2/src/pfr/pfr.c
        #libs/freetype2/src/pfr/pfrcmap.c
        #libs/freetype2/src/pfr/pfrdrivr.c
        #libs/freetype2/src/pfr/pfrgload.c
        #libs/freetype2/src/pfr/pfrload.c
        #libs/freetype2/src/pfr/pfrobjs.c
        #libs/freetype2/src/pfr/pfrsbit.c
    libs/freetype2/src/psaux/psaux.c
        #libs/freetype2/src/psaux/afmparse.c
        #libs/freetype2/src/psaux/psauxmod.c
        #libs/freetype2/src/psaux/psobjs.c
        #libs/freetype2/src/psaux/t1cmap.c
        #libs/freetype2/src/psaux/t1decode.c
        #libs/freetype2/src/psaux/cffdecode.c
        #libs/freetype2/src/psaux/psarrst.c
        #libs/freetype2/src/psaux/psblues.c
        #libs/freetype2/src/psaux/pserror.c
        #libs/freetype2/src/psaux/psfont.c
        #libs/freetype2/src/psaux/psft.c
        #libs/freetype2/src/psaux/pshints.c
        #libs/freetype2/src/psaux/psintrp.c
        #libs/freetype2/src/psaux/psread.c
        #libs/freetype2/src/psaux/psstack.c
    libs/freetype2/src/psaux/psconv.c
    libs/freetype2/src/pshinter/pshinter.c
        #libs/freetype2/src/pshinter/pshalgo.c
        #libs/freetype2/src/pshinter/pshglob.c
        #libs/freetype2/src/pshinter/pshmod.c
        #libs/freetype2/src/pshinter/pshrec.c
    libs/freetype2/src/psnames/psnames.c
        #libs/freetype2/src/psnames/psmodule.c
    libs/freetype2/src/raster/raster.c
        #libs/freetype2/src/raster/ftraster.c
        #libs/freetype2/src/raster/ftrend1.c
    libs/freetype2/src/sdf/sdf.c
        #libs/freetype2/src/sdf/ftsdfrend.c
        #libs/freetype2/src/sdf/ftsdfcommon.c
        #libs/freetype2/src/sdf/ftbsdf.c
        #libs/freetype2/src/sdf/ftsdf.c 
    libs/freetype2/src/sfnt/sfnt.c
        #libs/freetype2/src/sfnt/pngshim.c
        #libs/freetype2/src/sfnt/sfdriver.c
        #libs/freetype2/src/sfnt/sfobjs.c
        #libs/freetype2/src/sfnt/sfwoff.c
        #libs/freetype2/src/sfnt/sfwoff2.c
        #libs/freetype2/src/sfnt/ttbdf.c
        #libs/freetype2/src/sfnt/ttcmap.c
        #libs/freetype2/src/sfnt/ttcolr.c
        #libs/freetype2/src/sfnt/ttcpal.c
        #libs/freetype2/src/sfnt/ttkern.c
        #libs/freetype2/src/sfnt/ttload.c
        #libs/freetype2/src/sfnt/ttmtx.c
        #libs/freetype2/src/sfnt/ttpost.c
        #libs/freetype2/src/sfnt/ttsbit.c
        #libs/freetype2/src/sfnt/woff2tags.c

    libs/freetype2/src/smooth/smooth.c
        #libs/freetype2/src/smooth/ftgrays.c
        #libs/freetype2/src/smooth/ftsmooth.c

    #libs/freetype2/src/tools/apinames.c
    #libs/freetype2/src/tools/ftrandom/ftrandom.c
    #libs/freetype2/src/tools/test_afm.c
    #libs/freetype2/src/tools/test_bbox.c
    #libs/freetype2/src/tools/test_trig.c
    libs/freetype2/src/truetype/truetype.c
        #libs/freetype2/src/truetype/ttdriver.c
        #libs/freetype2/src/truetype/ttgload.c
        #libs/freetype2/src/truetype/ttgxvar.c
        #libs/freetype2/src/truetype/ttinterp.c
        #libs/freetype2/src/truetype/ttobjs.c
        #libs/freetype2/src/truetype/ttpload.c
        #libs/freetype2/src/truetype/ttsubpix.c
    libs/freetype2/src/type1/type1.c
        #libs/freetype2/src/type1/t1afm.c
        #libs/freetype2/src/type1/t1driver.c
        #libs/freetype2/src/type1/t1gload.c
        #libs/freetype2/src/type1/t1load.c
        #libs/freetype2/src/type1/t1objs.c
        #libs/freetype2/src/type1/t1parse.c
    libs/freetype2/src/type42/type42.c
        #libs/freetype2/src/type42/t42drivr.c
        #libs/freetype2/src/type42/t42objs.c
        #libs/freetype2/src/type42/t42parse.c
    libs/freetype2/src/winfonts/winfnt.c
)
target_include_directories(spfreetype2 PUBLIC libs/freetype2/include)
target_compile_definitions(spfreetype2 PRIVATE FT2_BUILD_LIBRARY)
set(FREETYPE_LIBRARIES spfreetype2)
set(FREETYPE_BUNDLED TRUE)

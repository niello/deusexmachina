//------------------------------------------------------------------------------
//  nmaketqt2.cc
//
//  Creates a tqt2 chunked texture file from a 32 bit TGA file.
//
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "ncmdlineargs.h"
#include "ntqt2filemaker.h"
#include "ntqt2compressor.h"

//------------------------------------------------------------------------------
/**
*/
int
main(int argc, const char** argv)
{
    nCmdLineArgs args(argc, argv);
    nKernelServer kernelServer;

    // get cmd line args
    bool helpArg = args.GetBoolArg("-help");
    nString inFile  = args.GetStringArg("-in", 0);
    nString outFile = args.GetStringArg("-out", 0);
    nString tmpFile = args.GetStringArg("-tmp", "home:tmp.tqt2");
    int depth           = args.GetIntArg("-depth", 4);
    int tileSize        = args.GetIntArg("-tilesize", 256);
    bool dxt1           = args.GetBoolArg("-dxt1");
    bool dxt5           = args.GetBoolArg("-dxt5");

    if (helpArg)
    {
        n_printf("(C) 2003 RadonLabs GmbH\n"
                 "Based on Thatcher Ulrich's maketqt tool\n\n"
				 "Modified by Vladimir \"Niello\" Orlov, 2011"
                 "-help     -- show this help\n"
                 "-in       -- input tga filename (size 2^n, 32 bits uncompressed), or tqt2 RAW file\n"
                 "-out      -- output filename\n"
                 "-tmp      -- filename of optional temporary file\n"
                 "-depth    -- tree depth (1..12, default 4)\n"
                 "-tilesize -- size of a base level tile (must be 2^n, default 256)\n"
                 "-dxt1     -- if present, compress input RAW tqt2 file to DDS tqt2 file (ignore alpha)\n"
                 "-dxt5     -- if present, compress input RAW tqt2 file to DDS tqt2 file (encode alpha)\n");
        return 5;
    }

    if (!inFile.IsValid())
    {
        n_printf("Input filename required [-in]!\n");
        return 10;
    }
    if (!outFile.IsValid())
    {
        n_printf("Output filename required [-out]!\n");
        return 10;
    }

    // configure and run a tqt2 filemaker
    bool compress = dxt1 | dxt5;
    if (!compress)
    {
        nTqt2FileMaker fileMaker(&kernelServer);
        fileMaker.SetSourceFile(inFile.Get());
        fileMaker.SetTargetFile(outFile.Get());
        fileMaker.SetTileSize(tileSize);
        fileMaker.SetTreeDepth(depth);
        if (!fileMaker.Run())
        {
            n_printf("TQT2 FileMaker failed with: %s\n", fileMaker.GetError());
            return 10;
        }
    }

    // convert the raw tqt2 file to a compressed tqt2 file
    if (compress)
    {
        n_printf("\n-> compressing generated texture tiles...\n");
        nTqt2Compressor compressor(&kernelServer);
        compressor.SetSourceFile(inFile.Get());
        if (dxt1)
        {
            compressor.SetMode(nTqt2Compressor::DXT1);
        }
        else
        {
            compressor.SetMode(nTqt2Compressor::DXT5);
        }
        compressor.SetTargetFile(outFile.Get());
        if (!compressor.Run())
        {
            n_printf("TQT2 Compressor failed with: %s\n", compressor.GetError());
            return 10;
        }
    }

    // done
    n_printf("-> Done.\n");
    return 0;
}

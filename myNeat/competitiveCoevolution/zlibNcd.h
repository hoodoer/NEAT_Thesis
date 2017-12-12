#ifndef ZLIBNCD_H
#define ZLIBNCD_H

#include <iostream>
#include <vector>
#include <zlib.h>

/********************************************************
   This simple class allows you to calculate a
   Normalized Compression Distance (NCD) between
   char* buffers. This class does not help with the
   actual serialization of data into buffers, you'll
   have to handle that yourself.

   This particular implementation uses the
   Zlib compression algorithm. This is a very
   fast compression algorithm, and is appropriate
   for use in cases where speed is of the utmost
   importance.

   The downside of using Zlib for NCD calculations
   is that the two buffers together cannot be bigger
   than 32K bytes. NCD won't work in this case.

   There are better NCD implementations. LZMA compression
   is an excellent NCD compressor to use, but is
   significantly slower than zlib. You avoid the
   size restrictions with LZMA, and it's very
   accurate.

   For more information on NCD, see www.complearn.org.
   They have excellent NCD implementations there.
   CompLearn.org is moving away from
   zlib and gzip implementations to
   a wholly LZMA implementation to save researchers from
   themselves, as numerous studies have ignored the size
   limits associated with various compressors.

   There is no .cpp file for this class, the
   implementation is completely contained in this
   .h file, so it's very easy to add to your software.

   By default, this class uses the fastest Zlib
   compression level. You can change that with
   the function ChangeCompressionLevel. Basically
   you can only choose Z_BEST_SPEED (default),
   or Z_BEST_COMPRESSION.

   Drew Kirkpatrick
   drew.kirkpatrick@gmail.com

********************************************************/


struct NcdDataObject
{
    char*  data;
    size_t size;
};





class NcdZlibCalc
{
    private:
    uLongf buffer1CompressedSize;
    uLongf buffer2CompressedSize;
    uLongf appendedBuffersCompressedSize;
    uLongf compressedSize;

    uLongf *minSize;
    uLongf *maxSize;

    size_t combinedBufferSize;
    char*  combinedBuffer;
    char*  compressedBuffer;

    int    zlibErrorCode;
    int    compressionLevel;

    // For doing multiple comparisons
    // in a row
    int    numComparisons;
    double ncdValue;
    int    loopCounter;


    inline void checkZlibErrorCode(int errorCode)
    {
        if (errorCode != Z_OK)
        {
            switch (errorCode)
            {
            case Z_MEM_ERROR:
                std::cout<<"NcdZlibCalc class, Zlib compress2 Memory error!"<<std::endl;
                exit(1);
                break;

            case Z_BUF_ERROR:
                std::cout<<"NcdZlibCalc class, Zlib compress2 buffer error!"<<std::endl;
                exit(1);
                break;

            case Z_STREAM_ERROR:
                std::cout<<"NcdZlibCalc class, Zlib compress2 stream error!"<<std::endl;
                exit(1);
                break;

            default:
                std::cout<<"NcdZlibCalc class, Zlib compress2 unknown error: "<<zlibErrorCode<<std::endl;
                exit(1);
                break;
            }
        }
    }


    public:
    // Set default compression level to fastest
    NcdZlibCalc()
    {
        compressionLevel = Z_BEST_SPEED;
    }


    // As of the time of writing, zlib supports
    // the following compression levels that
    // would work for NCD:
    // Z_BEST_SPEED (default) = 1
    // Z_BEST_COMPRESSION = 9
    void ChangeCompressionLevel(int newCompressionLevel)
    {
        if ((newCompressionLevel != Z_BEST_SPEED) &&
            (newCompressionLevel != Z_BEST_COMPRESSION))
        {
            std::cout<<"NcdZlibCalc class, invalid zlib compression level!"<<std::endl;
            exit(1);
        }

        compressionLevel = newCompressionLevel;
    }





    // This is the actual function that calculates NCD values
    inline double GetNcdValue(NcdDataObject object1, NcdDataObject object2)
    {
        // Step 1
        // Get the compressed size of object1
        compressedSize   = (uLongf)(object1.size + (object1.size * 0.1) + 12);
        compressedBuffer = NULL;
        compressedBuffer = new char[compressedSize]();

        zlibErrorCode = compress2((Bytef*)compressedBuffer, (uLongf*)&compressedSize,
                                  (const Bytef*)object1.data, (uLongf)object1.size, Z_BEST_SPEED);
        checkZlibErrorCode(zlibErrorCode);

        buffer1CompressedSize = compressedSize;
        delete[] compressedBuffer;



        // Step 2
        // Now get the compressed size of object2
        compressedSize   = (uLongf)(object2.size + (object2.size * 0.1) + 12);
        compressedBuffer = NULL;
        compressedBuffer = new char[compressedSize]();

        zlibErrorCode = compress2((Bytef*)compressedBuffer, (uLongf*)&compressedSize,
                                  (const Bytef*)object2.data, (uLongf)object2.size, Z_BEST_SPEED);
        checkZlibErrorCode(zlibErrorCode);

        buffer2CompressedSize = compressedSize;
        delete[] compressedBuffer;




        // Step 3
        // Now append the two buffers together and get their compressed size
        combinedBufferSize = object1.size + object2.size;
        combinedBuffer     = new char[combinedBufferSize]();

        memcpy(combinedBuffer, object1.data, object1.size);
        memcpy(combinedBuffer + object1.size, object2.data, object2.size);

        // Now get the compressed size of the appended buffers
        compressedSize = (uLongf)(combinedBufferSize + (combinedBufferSize * 0.1) + 12);



        // CHECK IF ZLIB WILL WORK!
        // This catenated buffer is the biggest, and we need to
        // first to make sure it's not too big for zlib sliding window
        // Going above 32K size will make NCD fail. If you need to
        // compare larger buffers of data, you need another compression
        // algorithm such as LZMA, or bzip2
        if (compressedSize > 32000)
        {
            std::cout<<"NcdZlibCalc class, combined buffer is too big for Zlib based NCD calculations!"<<std::endl;
            exit(1);
        }

        compressedBuffer = NULL;
        compressedBuffer = new char[compressedSize]();

        zlibErrorCode = compress2((Bytef*)compressedBuffer, (uLongf*)&compressedSize,
                                  (const Bytef*)combinedBuffer, (uLongf)combinedBufferSize, Z_BEST_SPEED);
        checkZlibErrorCode(zlibErrorCode);

        appendedBuffersCompressedSize = compressedSize;
        delete[] combinedBuffer;
        delete[] compressedBuffer;



        // Step 4
        // Calculate the NCD value
        // Need to know minimum and maximum compressed sizes
        if (buffer1CompressedSize < buffer2CompressedSize)
        {
            minSize = &buffer1CompressedSize;
            maxSize = &buffer2CompressedSize;
        }
        else
        {
            minSize = &buffer2CompressedSize;
            maxSize = &buffer1CompressedSize;
        }

        // This is the actual NCD equation.
        return ((double)(appendedBuffersCompressedSize - (double)*minSize) / (double)*maxSize);
    }




    // This function is just like above, except it compares
    // object1 to all the objects in the vector. The returned
    // NCD value is the average NCD value from all of the
    // comparisons.
    double GetNcdValue(NcdDataObject object1,
                              vector <NcdDataObject> objectsVector,
                              bool averageValues)
    {
        ncdValue       = 0.0;
        numComparisons = objectsVector.size();


        // Step 1
        // Get the compressed size of object1.
        // We'll hold onto this, because we'll use
        // this for the looped comparisons
        compressedSize   = (uLongf)(object1.size + (object1.size * 0.1) + 12);
        compressedBuffer = NULL;
        compressedBuffer = new char[compressedSize]();

        zlibErrorCode = compress2((Bytef*)compressedBuffer, (uLongf*)&compressedSize,
                                  (const Bytef*)object1.data, (uLongf)object1.size, Z_BEST_SPEED);
        checkZlibErrorCode(zlibErrorCode);

        buffer1CompressedSize = compressedSize;
        delete[] compressedBuffer;


        // Loop through all the objects in the vector for comparison, and
        // add up the NCD values for each comparison
        for (loopCounter = 0; loopCounter < numComparisons; loopCounter++)
        {
            // Step 2
            // Now get the compressed size of an object in the vector
            compressedSize   = (uLongf)(objectsVector[loopCounter].size + (objectsVector[loopCounter].size * 0.1) + 12);
            compressedBuffer = NULL;
            compressedBuffer = new char[compressedSize]();

            zlibErrorCode = compress2((Bytef*)compressedBuffer, (uLongf*)&compressedSize,
                                      (const Bytef*)objectsVector[loopCounter].data, (uLongf)objectsVector[loopCounter].size, Z_BEST_SPEED);
            checkZlibErrorCode(zlibErrorCode);

            buffer2CompressedSize = compressedSize;
            delete[] compressedBuffer;


            // Step 3
            // Now append the two buffers together and get their compressed size
            combinedBufferSize = object1.size + objectsVector[loopCounter].size;
            combinedBuffer     = new char[combinedBufferSize]();

            memcpy(combinedBuffer, object1.data, object1.size);
            memcpy(combinedBuffer + object1.size, objectsVector[loopCounter].data, objectsVector[loopCounter].size);

            // Now get the compressed size of the appended buffers
            compressedSize = (uLongf)(combinedBufferSize + (combinedBufferSize * 0.1) + 12);


            // CHECK IF ZLIB WILL WORK!
            // This catenated buffer is the biggest, and we need to
            // first to make sure it's not too big for zlib sliding window
            // Going above 32K size will make NCD fail (actually, the results
            // get worst and worst the more you go over the limi). If you need to
            // compare larger buffers of data, you need another compression
            // algorithm such as LZMA, or bzip2
            if (compressedSize > 32000)
            {
                std::cout<<"NcdZlibCalc class, combined buffer is too big for Zlib based NCD calculations!"<<std::endl;
                exit(1);
            }

            compressedBuffer = NULL;
            compressedBuffer = new char[compressedSize]();

            zlibErrorCode = compress2((Bytef*)compressedBuffer, (uLongf*)&compressedSize,
                                      (const Bytef*)combinedBuffer, (uLongf)combinedBufferSize, Z_BEST_SPEED);
            checkZlibErrorCode(zlibErrorCode);

            appendedBuffersCompressedSize = compressedSize;
            delete[] combinedBuffer;
            delete[] compressedBuffer;


            // Step 4
            // Calculate the NCD value
            // Need to know minimum and maximum compressed sizes
            if (buffer1CompressedSize < buffer2CompressedSize)
            {
                minSize = &buffer1CompressedSize;
                maxSize = &buffer2CompressedSize;
            }
            else
            {
                minSize = &buffer2CompressedSize;
                maxSize = &buffer1CompressedSize;
            }

            // This is the actual NCD equation.
            ncdValue += ((double)(appendedBuffersCompressedSize - (double)*minSize) / (double)*maxSize);
        }

        // Step 5, average the ncd value
        if (averageValues)
        {
            ncdValue /= (double)numComparisons;
        }

        return ncdValue;
    }


    // Overloaded functions to switch between averaged and unaveraged values
    double GetAveragedNcdValue(NcdDataObject object1,
                                     vector <NcdDataObject> objectsVector)
    {
        return GetNcdValue(object1, objectsVector, true);
    }

    double GetNonAveragedNcdValue(NcdDataObject object1,
                                         vector <NcdDataObject> objectsVector)
    {
        return GetNcdValue(object1, objectsVector, false);
    }
};


#endif // ZLIBNCD_H

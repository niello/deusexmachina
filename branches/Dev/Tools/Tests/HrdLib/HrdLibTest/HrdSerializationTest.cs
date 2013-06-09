using System;
using System.IO;
using HrdLib;
using NUnit.Framework;

namespace HrdLibTest
{
    [TestFixture]
    public class HrdSerializationTest
    {
        [Test]
        public void TestArraySerialization()
        {
            var testObj = new ArraySerializationTest
                {
                    Array = new[] {1, 2, 3, 4, 5},
                    ArrayOfArrays = new int[5][],
                    ThreeDimensionalArray = new int[5,5,5],
                    CompositeArray = new int[5][][,],
                    CompositeArray2 = new int[5][,][],
                    CompositeArray3 = new int[5,5][][],
                };

            int num = 0;
            for (int i = 0; i < testObj.ArrayOfArrays.Length; i++)
            {
                testObj.ArrayOfArrays[i] = new int[i];
                for (int j = 0; j < i; j++)
                    testObj.ArrayOfArrays[i][j] = num++;
            }
            testObj.ThreeDimensionalArray = new int[5,5,5];

            num = 0;
            for (int i = 0; i < testObj.ThreeDimensionalArray.GetLength(0); i++)
            {
                for (int j = 0; j < testObj.ThreeDimensionalArray.GetLength(1); j++)
                {
                    for (int k = 0; k < testObj.ThreeDimensionalArray.GetLength(2); k++)
                        testObj.ThreeDimensionalArray[i, j, k] = num++;
                }
            }

            num = 0;
            for (int i = 0; i < testObj.CompositeArray.Length; i++)
            {
                int d1 = 2*(i + 1), d2 = d1 + 1;
                testObj.CompositeArray[i] = new int[d1][,];
                testObj.CompositeArray2[i] = new int[d1,d2][];
                for (int j = 0; j < Math.Max(d1,5); j++)
                {
                    if (j < d1)
                        testObj.CompositeArray[i][j] = new int[d2,i];
                    if (j < 5)
                        testObj.CompositeArray3[i, j] = new int[d2][];
                    for (int k = 0; k < d2; k++)
                    {
                        if(j<d1)
                            testObj.CompositeArray2[i][j, k] = new int[i];
                        if (j < 5)
                            testObj.CompositeArray3[i, j][k] = new int[i];
                        for (int l = 0; l < i; l++)
                        {
                            if (j < d1)
                            {
                                testObj.CompositeArray[i][j][k, l] = num++;
                                testObj.CompositeArray2[i][j, k][l] = num++;
                            }
                            if (j < 5)
                                testObj.CompositeArray3[i, j][k][l] = num++;
                        }
                    }
                }
            }

            var path = Path.GetFullPath("ArrayTest.hrd");
            ArraySerializationTest deserialized;
            try
            {
                using (var stream = new FileStream(path, FileMode.Create, FileAccess.Write))
                {
                    HrdSerializer.Serialize(stream, testObj);
                    stream.Flush();
                }

                using (var stream = new FileStream(path, FileMode.Open, FileAccess.Read))
                {
                    deserialized = HrdSerializer.Deserialize<ArraySerializationTest>(stream);
                }
            }
            finally
            {
                if (File.Exists(path))
                    File.Delete(path);
            }
        }
    }
}

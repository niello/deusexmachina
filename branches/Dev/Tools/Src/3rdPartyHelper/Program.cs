using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Linq;
using System.Security.Cryptography;

namespace ThirdPartyHelper
{
    class Program
    {
        static void Main(string[] args)
        {
            var defaultColor = Console.ForegroundColor;
            if (args == null || args.Length == 0)
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("WARNING. Command line must contain a file name.");
                Console.ForegroundColor = defaultColor;
                return;
            }

            try
            {
                var downloadCaheFolder = args.Length > 1 ? args[1] : null;
                int flagIdx = 1;
                if (downloadCaheFolder != null && !downloadCaheFolder.StartsWith("--"))
                {
                    flagIdx++;
                    try
                    {
                        var folder = Path.GetFullPath(downloadCaheFolder);
                        if (!Directory.Exists(folder))
                            Directory.CreateDirectory(folder);
                        downloadCaheFolder = folder;
                    }
                    catch (Exception ex)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine("WARNING. Argument '{0}' is not valid: {1}.", downloadCaheFolder, ex.Message);
                        Console.ForegroundColor = defaultColor;
                        downloadCaheFolder = null;
                    }
                }

                bool cleanup = false;
                for (; flagIdx < args.Length; flagIdx++ )
                {
                    var flag = args[flagIdx];
                    if (flag == null || !flag.StartsWith("--"))
                        continue;

                    if (string.Compare(flag, "--cleanup", StringComparison.InvariantCultureIgnoreCase) == 0)
                        cleanup = true;
                }

                DownloadAndExtract(args[0], downloadCaheFolder, cleanup);
            }
            catch(Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("ERROR. {0}", ex);
                Console.ForegroundColor = defaultColor;
            }

#if DEBUG
            Console.WriteLine("Press any key to quit.");
            Console.ReadKey(false);
#endif
        }

        static void DownloadAndExtract(string fileName, string caheFolder, bool cleanup)
        {   
            var file = Path.GetFullPath(fileName);
            if (!File.Exists(file))
                throw new FileNotFoundException("File is not found.", file);

            var downloadList=new List<DownloadInfo>();
            using(var stream=new FileStream(file,FileMode.Open,FileAccess.Read))
            {
                var reader = new StreamReader(stream);
                while(!reader.EndOfStream)
                {
                    var line = reader.ReadLine();
                    if (string.IsNullOrEmpty(line))
                        continue;

                    var downloadInfo = DownloadInfo.Parse(line);
                    downloadList.Add(downloadInfo);
                }
            }

            if (cleanup)
                Cleanup(downloadList);

            var shell = new ShellClass();
            var md5 = MD5.Create();

            foreach (var item in downloadList)
            {
                bool downloaded = false, keepFile = false;
                string tempFile;
                if (caheFolder != null && !string.IsNullOrEmpty(item.CacheFileName))
                {
                    tempFile = Path.GetFullPath(Path.Combine(caheFolder, item.CacheFileName));
                    var tempDir = Path.GetDirectoryName(tempFile);
                    Debug.Assert(!string.IsNullOrEmpty(tempDir));
                    if (!Directory.Exists(tempDir))
                        Directory.CreateDirectory(tempDir);
                    else if (File.Exists(tempFile))
                    {
                        using (var fStream=new FileStream(tempFile,FileMode.Open,FileAccess.Read))
                        {
                            var hash = md5.ComputeHash(fStream);
                            var hashString = BitConverter.ToString(hash).Replace("-", string.Empty).ToLowerInvariant();
                            downloaded = hashString == item.HashCode;
                            if (downloaded)
                                Console.WriteLine("The file from {0} has been found in the cache.", item.Uri);
                        }
                    }
                    keepFile = true;
                }
                else
                    tempFile = Path.GetTempFileName() + ".zip";

                if (!downloaded)
                {
                    HttpGet(item.Uri, tempFile);

                    using (var fStream = new FileStream(tempFile, FileMode.Open, FileAccess.Read))
                    {
                        var hash = md5.ComputeHash(fStream);
                        var hashString = BitConverter.ToString(hash).Replace("-", string.Empty).ToLowerInvariant();
                        var color = Console.ForegroundColor;
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        if (string.IsNullOrEmpty(item.HashCode))
                        {
                            Console.WriteLine(
                                "WARNING. Hash code is not defined for the file. Hash code check will be skipped.");
                            Console.WriteLine("Hash code: {0}", hashString);
                        }
                        else if (!string.Equals(item.HashCode, hashString, StringComparison.InvariantCultureIgnoreCase))
                        {
                            Console.WriteLine("WARNING. Hash code mismatch.");
                            Console.WriteLine("Hash code: {0}. Required hash code: {1}.", hashString, item.HashCode);
                        }
                        Console.ForegroundColor = color;
                    }
                }

                UnzipAll(shell, tempFile, item.SourcePath, item.DestinationPath);

                if (!keepFile)
                {
                    try
                    {
                        File.Delete(tempFile);
                    }
                    catch
                    {
                        var color = Console.ForegroundColor;
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine("WARNING. Can't delete temporary file '{0}'.", tempFile);
                        Console.ForegroundColor = color;
                    }
                }
            }

        }

        private static void Cleanup(ICollection<DownloadInfo> downloadList)
        {
            var folderList = new List<string>(downloadList.Count);
            foreach(var download in downloadList)
            {
                var folderPath =
                    Path.GetFullPath(download.DestinationPath).ToLower().TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                bool ignore = false;
                for(int i=0; i<folderList.Count; i++)
                {
                    if (folderList[i] == null)
                        continue;

                    if(folderPath.StartsWith(folderList[i]))
                    {
                        ignore = true;
                        break;
                    }
                    if (!folderList[i].StartsWith(folderPath))
                        continue;

                    folderList[i] = ignore ? null : folderPath;
                    ignore = true;
                }
                if (!ignore)
                    folderList.Add(folderPath);
            }

            var svnProcessStart =
                new ProcessStartInfo("svn.exe", "status --no-ignore")
                    {
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                    };

            foreach(var folder in folderList.Where(f=>f!=null && Directory.Exists(f)))
            {
                svnProcessStart.Arguments = string.Format("status \"{0}\" --no-ignore", folder);
                using (var process = new Process())
                {
                    process.StartInfo = svnProcessStart;
                    var currentFolder = folder;

                    var toDelete = new List<string>();
                    process.OutputDataReceived +=
                        (o, e) =>
                            {
                                var line = e.Data;
                                if (line == null)
                                    return;

                                if (!line.StartsWith("I") || line.Length < 1)
                                    return;

                                line = line.Substring(1).Trim();
                                if (!string.IsNullOrEmpty(line))
                                    toDelete.Add(line);
                            };

                    process.ErrorDataReceived +=
                        (o, e) =>
                            {
                                if (e.Data == null)
                                    return;

                                var color = Console.ForegroundColor;
                                Console.ForegroundColor = ConsoleColor.Yellow;
                                Console.WriteLine(
                                    "WARNING. Can't cleanup folder '{0}' because of the following svn error:",
                                    currentFolder);
                                Console.ForegroundColor = ConsoleColor.Red;
                                Console.WriteLine(e.Data);
                                Console.ForegroundColor = color;
                            };

                    process.Start();
                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();
                    process.WaitForExit();

                    foreach (var path in toDelete)
                    {
                        if (File.Exists(path))
                        {
                            File.Delete(path);
                            Console.WriteLine("Cleanup: file '{0}' was deleted.", path);
                        }
                        else if (Directory.Exists(path))
                        {
                            Directory.Delete(path, true);
                            Console.WriteLine("Cleanup: directory '{0}' was deleted with an all its content.", path);
                        }
                    }
                }
            }
        }

        static void HttpGet(string uri, string destination)
        {
            Console.WriteLine("Downloading file from {0}", uri);
            var request = (HttpWebRequest)WebRequest.Create(uri);
            request.Method = WebRequestMethods.Http.Get;
            
            using (var response = (HttpWebResponse)request.GetResponse())
            using (var getStream = response.GetResponseStream())
            {
                if (getStream == null)
                    throw new InvalidOperationException(string.Format("There was no response from '{0}'.", uri));

                using (var fStream = new FileStream(destination, FileMode.Create, FileAccess.Write))
                {
                    var buffer = new byte[2048];
                    int length;
                    do
                    {
                        
                        length = getStream.Read(buffer, 0, buffer.Length);
                        fStream.Write(buffer, 0, length);
                    } while (length > 0);
                }
            }
        }

        private static void UnzipAll(ShellClass shell, string zippedFile, string sourcePath, string destinationFolder)
        {
            var destinationPath = Path.GetFullPath(destinationFolder);
            Console.WriteLine("Extracting files to {0}", destinationPath);

            if (!Directory.Exists(destinationPath))
                Directory.CreateDirectory(destinationPath);

            var source = Path.Combine(Path.GetFullPath(zippedFile), sourcePath);

            var from = shell.NameSpace(source);
            var to = shell.NameSpace(destinationPath);

            to.CopyHere(from.Items(), FileOperationFlag.NoConfirmation);
        }
    }
}

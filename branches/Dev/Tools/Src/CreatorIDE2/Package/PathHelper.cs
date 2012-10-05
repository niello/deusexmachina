using System;
using System.IO;
using System.Linq;
using System.Text;

namespace CreatorIDE.Package
{
    public static class PathHelper
    {
        public static string GetRelativePath(string baseDirectory, string fullPath)
        {
            if(baseDirectory==null)
                throw new ArgumentNullException("baseDirectory");
            if (fullPath == null)
                throw new ArgumentNullException("fullPath");

            var targetPath = fullPath.Split('\\');
            var basePath = baseDirectory.Split('\\');

            int minLen = Math.Min(targetPath.Length, basePath.Length);
            int i;
            for (i = 0; i < minLen; i++)
            {
                if (!string.Equals(targetPath[i], basePath[i], StringComparison.OrdinalIgnoreCase))
                {
                    if(i==0)
                        return fullPath;

                    break;
                }
            }

            var relPath = new StringBuilder();
            for (int j = i; j < basePath.Length; j++)
            {
                if (j != i)
                    relPath.Append('\\');
                relPath.Append("..");
            }

            for (int j = i; j < targetPath.Length; j++)
            {
                if (relPath.Length > 0)
                    relPath.Append('\\');
                relPath.Append(targetPath[j]);
            }

            if (relPath.Length == 0)
                relPath.Append('.');

            return relPath.ToString();
        }

        public static string Combine(string path, params string[] paths)
        {
            if (path == null)
                throw new ArgumentNullException("path");

            if (paths == null || paths.Length == 0)
                return path;

            return paths.Aggregate(path, Path.Combine);
        }

        public static string GetFullPath(string path, params string[] paths)
        {
            var combinedPath = Combine(path, paths);
            return Path.GetFullPath(combinedPath);
        }
    }
}

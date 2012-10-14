using System;
using System.Linq;
using System.Text.RegularExpressions;

namespace ThirdPartyHelper
{
    class DownloadInfo
    {
        private static readonly Regex StringPattern = new Regex(@"""[^""]*""|[^""\s]*");

        public string Uri { get; set; }
        public string HashCode { get; set; }
        public string SourcePath { get; set; }
        public string DestinationPath { get; set; }

        public static DownloadInfo Parse(string str)
        {
            var matches = StringPattern.Matches(str).Cast<Match>().Where(m => m.Length > 0).ToArray();
            if (matches.Length != 4)
                throw new InvalidOperationException(string.Format("String '{0}' does not match the following pattern: '\"Uri\" \"HashCode\" \"SourcePath\" \"DestinationPath\"", str));

            var result = new DownloadInfo();
            for(int i=0; i<4; i++)
            {
                var value = matches[i].Value.Trim(' ', '"');
                switch (i)
                {
                    case 0:
                        result.Uri = value;
                        break;
                    case 1:
                        result.HashCode = value;
                        break;
                    case 2:
                        result.SourcePath = value;
                        break;
                    case 3:
                        result.DestinationPath = value;
                        break;
                }
            }

            return result;
        }
    }
}

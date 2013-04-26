using System.IO;
using System.Text;

namespace HrdLib
{
    public class NotDisposingStreamWriter : StreamWriter
    {
        public NotDisposingStreamWriter(Stream stream, Encoding encoding) :
            base(stream, encoding)
        {
        }

        protected override void Dispose(bool disposing)
        {
            Flush();
            base.Dispose(false);
        }
    }
}

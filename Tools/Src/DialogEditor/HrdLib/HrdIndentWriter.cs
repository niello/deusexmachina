using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text;

namespace HrdLib
{
    internal sealed class HrdIndentWriter : IDisposable
    {
        private static readonly Encoding Encoding = Encoding.UTF8;

        private readonly char _indentChar;
        private readonly int _indentWidth;
        private readonly TextWriter _writer;

        private int _indent;
        private bool _isNewLine;

        public HrdIndentWriter(string fileName, char indentChar, int indentWidth)
        {
            if (fileName == null)
                throw new ArgumentNullException("fileName");

            _indentChar = indentChar;
            _indentWidth = indentWidth;

            FileStream fs = null;
            try
            {
                fs = new FileStream(fileName, FileMode.Create, FileAccess.ReadWrite);
                _writer = new StreamWriter(fs, Encoding);
            }
            catch
            {
                if (fs != null)
                    fs.Dispose();
                throw;
            }
        }

        public HrdIndentWriter(char indentChar, int indentWidth)
        {
            _indentChar = indentChar;
            _indentWidth = indentWidth;

            _writer = new StringWriter(new StringBuilder(), CultureInfo.InvariantCulture);
        }

        public void IncreaseIndent()
        {
            _indent++;
        }

        public bool DecreaseIndent()
        {
            if (_indent == 0)
                return false;

            _indent--;
            Debug.Assert(_indent >= 0);
            return true;
        }

        public HrdIndentWriter Write(string text)
        {
            if (text == null)
                return this;

            WriteIndent();
            _writer.Write(text);

            return this;
        }

        public HrdIndentWriter Write(IFormatProvider formatProvider, string formatString, params object[] args)
        {
            var text = string.Format(formatProvider, formatString, args);
            return Write(text);
        }

        public HrdIndentWriter Write(string formatString, params object[] args)
        {
            return Write(CultureInfo.InvariantCulture, formatString, args);
        }

        public HrdIndentWriter WriteLine()
        {
            WriteIndent();
            _writer.Write(Environment.NewLine);
            _isNewLine = true;
            return this;
        }

        public HrdIndentWriter WriteLine(string text)
        {
            return Write(text).WriteLine();
        }

        public HrdIndentWriter WriteLine(IFormatProvider formatProvider, string formatString, params object[] args)
        {
            return Write(formatProvider, formatString, args).WriteLine();
        }

        public HrdIndentWriter WriteLine(string formatString, params object[] args)
        {
            return WriteLine(CultureInfo.InvariantCulture, formatString, args);
        }

        private void WriteIndent()
        {
            if (!_isNewLine)
                return;

            _isNewLine = false;
            if (_indent == 0)
                return;

            Debug.Assert(_indent > 0);

            _writer.Write(new string(_indentChar, _indent*_indentWidth));
        }

        public string GetCode()
        {
            _writer.Flush();
            if (_writer is StreamWriter)
            {
                var sw = (StreamWriter) _writer;
                var positition = sw.BaseStream.Position;
                sw.BaseStream.Position = 0;

                var buffer = new byte[positition];
                int length = sw.BaseStream.Read(buffer, 0, buffer.Length);
                Debug.Assert(length == buffer.Length);

                var res = Encoding.GetString(buffer);
                sw.BaseStream.Position = positition;
                return res;
            }

            var stringWriter = (StringWriter) _writer;
            return stringWriter.GetStringBuilder().ToString();
        }

        public void WriteBeginElement(string elementName)
        {
            Write("writer.WriteBeginElement(");
            if (elementName != null)
                Write(string.Concat("\"", elementName, "\""));
            WriteLine(");");
        }

        public void WriteEndElement()
        {
            WriteLine("writer.WriteEndElement();");
        }

        public void WriteBeginArray(string arrayName)
        {
            Write("writer.WriteBeginArray(");
            if (arrayName != null)
                Write(string.Concat("\"", arrayName, "\""));
            WriteLine(");");
        }

        public void WriteEndArray()
        {
            WriteLine("writer.WriteEndArray();");
        }

        public void Flush()
        {
            _writer.Flush();
        }

        public void Dispose()
        {
            Dispose(true);
        }

        private void Dispose(bool disposing)
        {
            if (disposing)
                GC.SuppressFinalize(this);

            _writer.Dispose();
        }

        ~HrdIndentWriter()
        {
            Dispose(false);
        }
    }
}

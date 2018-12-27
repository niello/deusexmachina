using System;
using System.Linq;

namespace DialogLogic
{
    public abstract class DialogGraphPhraseNodeBase:DialogGraphNodeBase
    {
        private const int DisplayLength = 42;

        private string _phrase;
        public string Phrase
        {
            get { return _phrase; }
            set
            {
                if (_phrase != value)
                {
                    _phrase = value;
                    OnPropertyChanged("Phrase");
                }
            }
        }

        private string _character;
        public string Character
        {
            get { return _character; }
            set
            {
                if (_character != value)
                {
                    _character = value;
                    OnPropertyChanged("Character");
                }
            }
        }

        public override string DisplayName
        {
            get
            {
                var str = string.Format("{0}: {1}", Character, Phrase).Take(DisplayLength + 1).ToArray();
                if (str.Length == DisplayLength + 1)
                {
                    Array.Resize(ref str, DisplayLength + 3);
                    str[DisplayLength] = str[DisplayLength + 1] = str[DisplayLength + 2] = '.';
                }
                return new string(str);
            }
        }
    }
}

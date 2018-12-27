using System.ComponentModel;

namespace DialogLogic
{
    public class DialogGraphLink:INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        private string _condition;
        public string Condition
        {
            get { return _condition; }
            set
            {
                if (_condition != value)
                {
                    _condition = value;
                    OnPropertyChanged("Condition");
                }
            }
        }

        private string _action;
        public string Action
        {
            get { return _action; }
            set
            {
                if (_action != value)
                {
                    _action = value;
                    OnPropertyChanged("Action");
                }
            }
        }

        private readonly DialogGraphLinkDirection _direction;
        public DialogGraphLinkDirection Direction { get { return _direction; } }

        public DialogGraphLink(int parentNodeId, int? childNodeId)
        {
            _direction = new DialogGraphLinkDirection(parentNodeId, childNodeId);
        }

        private void OnPropertyChanged(string propertyName)
        {
            var eh = PropertyChanged;
            if(eh!=null)
            {
                eh(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}

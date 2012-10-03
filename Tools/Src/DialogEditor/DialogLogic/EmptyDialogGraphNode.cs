namespace DialogLogic
{
    public class EmptyDialogGraphNode:DialogGraphNodeBase
    {
        public override DialogGraphNodeType NodeType
        {
            get { return DialogGraphNodeType.Empty; }
        }

        public override string DisplayName
        {
            get
            {
                if(Id==-1)
                    return "[Root]";

                return string.Format("[Empty #{0}]", Id);
            }
        }

    }
}

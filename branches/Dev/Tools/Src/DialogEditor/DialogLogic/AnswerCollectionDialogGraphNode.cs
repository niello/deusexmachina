namespace DialogLogic
{
    public class AnswerCollectionDialogGraphNode:DialogGraphPhraseNodeBase
    {
        public override DialogGraphNodeType NodeType
        {
            get { return DialogGraphNodeType.Answers; }
        }
    }
}

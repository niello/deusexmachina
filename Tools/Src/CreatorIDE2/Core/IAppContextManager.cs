namespace CreatorIDE.Core
{
    public interface IAppContextManager
    {
        T GetContext<T>()
            where T : class;

        bool TryGetContext<T>(out T context)
            where T : class;
    }
}

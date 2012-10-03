Для упрощения процесса настройки сборки используется файл DialogEditor.csproj.targets, который является
стандартным файлом MSBuild (точно c таким же синтаксисом, как и все остальные файлы *.csproj). На данный
момент этот файл подключен ко всем проектам, и в нём определены следующие параметры сборки:
1. Каталог, куда производится сборка всех проектов;
2. Общий номер версии (файл DialogEditor.Version.cs) и общая для всех сборок информация (DialogEditor.AssemblyInfo.cs).

Файл DialogEditor.csproj.targets подключается путём следующей модификации файла проекта (*.csproj):

<Project>
...
  <!--Следующая строка включает в процесс сборки файл DialogEditor.csproj.targets. Обратите внимание, что все
      манипуляции с путями должны быть произведены до того, как будет подключен файл Microsoft.CSharp.targets-->
  <Import Condition="Exists('$(SolutionDir)$(SolutionName).csproj.targets')" Project="$(SolutionDir)$(SolutionName).csproj.targets" />

  <!--Следующая строка генерируется VS по-умолчанию для всех проектов и, опять же по-умолчанию, находится в конце
      файла проекта (*.csproj)-->
  <Import Project="$(MSBuildToolsPath)\Microsoft.CSharp.targets" />
...
</Project>
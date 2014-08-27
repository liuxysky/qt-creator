#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsoutputwindow.h>
#ifdef WITH_TESTS
#include <QTest>
#endif

namespace Mercurial {
namespace Internal {

using namespace VcsBase::Constants;
using namespace Mercurial::Constants;

    FILELOG_ID,
    FILELOG_DISPLAY_NAME,
    FILELOG,
    LOGAPP},
    const auto widgetCreator = []() { return new MercurialEditorWidget; };
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, m_client, describeSlot));
    auto cloneWizardFactory = new BaseCheckoutWizardFactory;
    cloneWizardFactory->setId(QLatin1String(VcsBase::Constants::VCS_ID_MERCURIAL));
    cloneWizardFactory->setIcon(QIcon(QLatin1String(":/mercurial/images/hg.png")));
    cloneWizardFactory->setDescription(tr("Clones a Mercurial repository and tries to load the contained project."));
    cloneWizardFactory->setDisplayName(tr("Mercurial Clone"));
    cloneWizardFactory->setWizardCreator([this] (const FileName &path, QWidget *parent) {
        return new CloneWizard(path, parent);
    });
    addAutoReleasedObject(cloneWizardFactory);
        VcsOutputWindow::appendError(tr("There are no changes to commit."));
        VcsBase::VcsOutputWindow::appendError(saver.errorString());
        VcsOutputWindow::appendError(tr("Unable to create an editor for the commit."));
    QTC_ASSERT(submitEditor(), return);
    Core::EditorManager::closeDocument(submitEditor()->document());
    MercurialEditorWidget editor;
    editor.setParameters(editorParameters + 2);
    MercurialEditorWidget editor;
    editor.setParameters(editorParameters);
} // namespace Internal
} // namespace Mercurial
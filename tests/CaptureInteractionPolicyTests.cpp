#include <QtTest>
#include "capture/CaptureInteractionPolicy.h"
#include "core/TranslationManager.h"

class CaptureInteractionPolicyTests : public QObject {
    Q_OBJECT
private slots:
    void resizeHandleReleasesActiveTool()
    {
        QVERIFY(shouldReleaseToolForResize(true, 3, 0));
        QVERIFY(!shouldReleaseToolForResize(false, 3, 0));
        QVERIFY(!shouldReleaseToolForResize(true, 0, 0));
    }

    void toolPersistenceDefaultsToNone()
    {
        QCOMPARE(initialAnnotationTool(false, 5, 0), 0);
        QCOMPARE(initialAnnotationTool(true, 5, 0), 5);
    }

    void annotationPersistenceLabelsExistInEveryLanguage()
    {
        for (int language = 0; language < TranslationManager::LangCount; ++language) {
            TranslationManager::setLanguage(static_cast<TranslationManager::Language>(language));
            QVERIFY(!TranslationManager::rememberLastAnnotationTool().isEmpty());
            QVERIFY(!TranslationManager::rememberLastAnnotationToolHint().isEmpty());
            QVERIFY(!TranslationManager::drawingTools().isEmpty());
            QVERIFY(!TranslationManager::bottomToolbarControls().isEmpty());
        }
        TranslationManager::setLanguage(TranslationManager::Turkish);
        QVERIFY(TranslationManager::rememberLastAnnotationTool().contains(QStringLiteral("ı")));
    }

    void auditedTranslationsDoNotFallBackToEnglish()
    {
        const QList<QByteArray> keys = {
            "ocrLanguagePackMissing", "uploadAuthHelpYandex",
            "uploadAuthHelpGoogleDrive", "uploadAuthHelpApiKey",
            "uploadErrorYandexScopeMissing", "uploadErrorGoogleAuthFailed",
            "uploadErrorApiKeyMissing", "toolFontSize"
        };
        TranslationManager::setLanguage(TranslationManager::English);
        QHash<QByteArray, QString> english;
        for (const QByteArray &key : keys) english.insert(key, TranslationManager::tr(key.constData()));
        for (int language = TranslationManager::German; language < TranslationManager::LangCount; ++language) {
            TranslationManager::setLanguage(static_cast<TranslationManager::Language>(language));
            for (const QByteArray &key : keys)
                QVERIFY2(TranslationManager::tr(key.constData()) != english.value(key), key.constData());
        }
    }
};

QTEST_APPLESS_MAIN(CaptureInteractionPolicyTests)
#include "CaptureInteractionPolicyTests.moc"

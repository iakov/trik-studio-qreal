/* Copyright 2007-2016 QReal Research Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#include "xmlCompiler.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include <qrutils/outFile.h>
#include <qrutils/xmlUtils.h>
#include <qrutils/stringUtils.h>

#include "editor.h"
#include "nameNormalizer.h"
#include "diagram.h"
#include "type.h"
#include "edgeType.h"
#include "nodeType.h"
#include "portType.h"
#include "enumType.h"

using namespace utils;

XmlCompiler::XmlCompiler()
{
	mResources = "<!DOCTYPE RCC><RCC version=\"1.0\">\n<qresource>\n";

	QDir dir;
	if (!dir.exists("generated")) {
		dir.mkdir("generated");
	}
	dir.cd("generated");
	if (!dir.exists("shapes")) {
		dir.mkdir("shapes");
	}
	dir.cd("..");
}

XmlCompiler::~XmlCompiler()
{
	qDeleteAll(mEditors);
}

bool XmlCompiler::compile(const QString &inputXmlFileName, const QString &sourcesRootFolder)
{
	const QFileInfo inputXmlFileInfo(inputXmlFileName);
	mPluginName = NameNormalizer::normalize(inputXmlFileInfo.completeBaseName());
	mCurrentEditor = inputXmlFileInfo.absoluteFilePath();
	mSourcesRootFolder = sourcesRootFolder;
	const QDir startingDir = inputXmlFileInfo.dir();
	if (!loadXmlFile(startingDir, inputXmlFileInfo.fileName())) {
		return false;
	}

	mPluginVersion = mEditors[mCurrentEditor]->version();

	generateCode();
	return true;
}

Editor* XmlCompiler::loadXmlFile(const QDir &currentDir, const QString &inputXmlFileName)
{
	QFileInfo fileInfo(inputXmlFileName);
	Q_ASSERT(fileInfo.fileName() == inputXmlFileName);

	QString fullFileName = currentDir.absolutePath() + "/" + inputXmlFileName;
	qDebug() << "Loading file started: " << fullFileName;

	if (mEditors.contains(fullFileName)) {
		Editor *editor = mEditors[fullFileName];
		if (editor->isLoaded()) {
			qDebug() << "File already loaded";
			return editor;
		} else {
			qDebug() << "ERROR: cycle detected";
			return nullptr;
		}
	} else {
		QString errorMessage;
		int errorLine = 0;
		int errorColumn = 0;
		QDomDocument inputXmlDomDocument = xmlUtils::loadDocument(fullFileName
			, &errorMessage, &errorLine, &errorColumn);
		if (!errorMessage.isEmpty()) {
			qCritical() << QString("(%1, %2):").arg(errorLine).arg(errorColumn)
					<< "Could not parse XML. Error:" << errorMessage;
		}

		Editor *editor = new Editor(inputXmlDomDocument, this);
		if (!editor->load(currentDir)) {
			qDebug() << "ERROR: Failed to load file";
			delete editor;
			return nullptr;
		}
		mEditors[fullFileName] = editor;
		return editor;
	}
}

Diagram * XmlCompiler::getDiagram(const QString &diagramName)
{
	foreach (Editor *editor, mEditors) {
		Diagram *diagram = editor->findDiagram(diagramName);
		if (diagram) {
			return diagram;
		}
	}
	return nullptr;
}

void XmlCompiler::generateCode()
{
	if (!mEditors.contains(mCurrentEditor)) {
		qDebug() << "ERROR: Main editor xml was not loaded, generation aborted";
		return;
	}

	generateElementClasses();
	generatePluginHeader();
	generatePluginSource();
	generateResourceFile();
}

void XmlCompiler::addResource(const QString &resourceName)
{
	if (!mResources.contains(resourceName))
		mResources += resourceName;
}

void XmlCompiler::generateElementClasses()
{
	OutFile outElements("generated/elements.h");

	generateAutogeneratedDisclaimer(outElements);

	outElements() << "#pragma once\n\n"
		<< "#include <QBrush>\n"
		<< "#include <QPainter>\n\n"
		<< "#include <qrgraph/queries.h>\n"
		<< "#include <metaMetaModel/nodeElementType.h>\n"
		<< "#include <metaMetaModel/edgeElementType.h>\n"
		<< "#include <metaMetaModel/patternType.h>\n"
		<< "#include <metaMetaModel/labelProperties.h>\n\n"
		;

	for (const Diagram *diagram : mEditors[mCurrentEditor]->diagrams().values()) {
		for (Type * const type : diagram->types().values()) {
			type->generateCode(outElements);
		}
	}
}

void XmlCompiler::generatePluginHeader()
{
	QString fileName = "generated/pluginInterface.h";// mPluginName

	OutFile out(fileName);

	generateAutogeneratedDisclaimer(out);

	out() << "#pragma once\n"
		<< "\n"
		<< "#include <metaMetaModel/metamodel.h>\n"
		<< "\n"
		<< "class " << mPluginName << "Plugin : public QObject, public qReal::Metamodel\n"
		<< "{\n\tQ_OBJECT\n\tQ_INTERFACES(qReal::Metamodel)\n"
		<< "\tQ_PLUGIN_METADATA(IID \"" << mPluginName << "\")\n"
		<< "\n"
		<< "public:\n"
		<< "\n"
		<< "\t" << mPluginName << "Plugin();\n"
		<< "\n"
		<< "\tQString id() const { return \"" << mPluginName << "\"; }\n"
		<< "\tQString version() const { return \"" << mPluginVersion << "\"; }\n"
		<< "\n"
		<< "\tQStringList diagrams() const override;\n"
		<< "\n"
		<< "\tQStringList getTypesContainedBy(const QString &element) const override;\n"
		<< "\tQList<QPair<QPair<QString, QString>, QPair<bool, QString>>> getPossibleEdges(QString "
				"const &element) const override;\n"
		<< "\tQList<qReal::Metamodel::ExplosionData> explosions(const QString &diagram, QString "
				"const &element) const override;\n"
		<< "\n"
		<< "\tQList<QPair<QString, QString>> enumValues(const QString &name) const override;\n"
		<< "\tbool isEnumEditable(const QString &name) const override;\n"
		<< "\n"
		<< "\tQString editorName() const override;\n"
		<< "\tQString diagramName(const QString &diagram) const override;\n"
		<< "\tqReal::ElementType *diagramNode(const QString &diagram) const override;\n"
		<< "\n"
		<< "\tQStringList diagramPaletteGroups(const QString &diagram) const override;\n"
		<< "\tQStringList diagramPaletteGroupList(const QString &diagram, const QString &group) const override;\n"
		<< "\tQString diagramPaletteGroupDescription(const QString &diagram, const QString &group) const override;\n"
		<< "\tbool shallPaletteBeSorted(const QString &diagram) const override;\n"
		<< "\n"
		<< "private:\n"
		<< "\tvirtual void initPlugin();\n"
		<< "\tvirtual void initMultigraph();\n"
		<< "\tvirtual void initNameMap();\n"
		<< "\tvirtual void initPaletteGroupsMap();\n"
		<< "\tvirtual void initPaletteGroupsDescriptionMap();\n"
		<< "\tvirtual void initShallPaletteBeSortedMap();\n"
		<< "\tvirtual void initExplosionsMap();\n"
		<< "\n"
		<< "\tQMap<QString, QString> mDiagramNameMap;\n"
		<< "\tQMap<QString, QString> mDiagramNodeNameMap;\n"
		<< "\tQMap<QString, QList<QPair<QString, QStringList>>> mPaletteGroupsMap;  // Maps element`s lists of all "
				"palette groups.\n"
		<< "\tQMap<QString, QMap<QString, QString>> mPaletteGroupsDescriptionMap;\n"
		<< "\tQMap<QString, bool> mShallPaletteBeSortedMap;\n"
		<< "\tQMap<QString, QMap<QString, QList<qReal::Metamodel::ExplosionData>>> mExplosionsMap;\n"
		<< "};\n"
		<< "\n";
}

void XmlCompiler::generatePluginSource()
{
	QString fileName = "generated/pluginInterface.cpp"; //mPluginName

	OutFile out(fileName);

	generateAutogeneratedDisclaimer(out);
	generateIncludes(out);
	generateInitPlugin(out);
	generateNameMappingsRequests(out);
	generateContainedTypes(out);
	generatePossibleEdges(out);
	generateEnumValues(out);
	generateEditableEnums(out);
}

void XmlCompiler::generateAutogeneratedDisclaimer(OutFile &out)
{
	out()
			<< "// ----------------------------------------------------------------------- //\n"
			<< "// This file is auto-generated with qrxc v1.1. Do not modify its contents\n"
			<< "// or prepare to loose your edits. If you want to change something in it\n"
			<< "// consider editing language metamodel instead.\n"
			<< "// ----------------------------------------------------------------------- //\n\n";
}

void XmlCompiler::generateIncludes(OutFile &out)
{
	out() << "#include \"" << "pluginInterface.h\"\n" //mPluginName
		<< "\n";

	out() << "#include \"" << "elements.h" << "\"\n";

	out() << "\n";

	out()
		//<< "Q_EXPORT_PLUGIN2(qreal_editors, " << mPluginName << "Plugin)\n\n"
		<< mPluginName << "Plugin::" << mPluginName << "Plugin()\n{\n"
		<< "\tinitPlugin();\n"
		<< "}\n\n";
}

void XmlCompiler::generateInitPlugin(OutFile &out)
{
	out() << "void " << mPluginName << "Plugin::initPlugin()\n{\n"
		<< "\tinitMultigraph();\n"
		<< "\tinitNameMap();\n"
		<< "\tinitPaletteGroupsMap();\n"
		<< "\tinitPaletteGroupsDescriptionMap();\n"
		<< "\tinitShallPaletteBeSortedMap();\n"
		<< "\tinitExplosionsMap();\n"
		<< "}\n\n";

	generateInitMultigraph(out);
	generateNameMappings(out);
	generatePaletteGroupsLists(out);
	generatePaletteGroupsDescriptions(out);
	generateShallPaletteBeSorted(out);
	generateExplosionsMappings(out);
}

void XmlCompiler::generateInitMultigraph(OutFile &out)
{
	out() << "void " << mPluginName << "Plugin::initMultigraph()\n{\n";

	for (const Diagram *diagram : mEditors[mCurrentEditor]->diagrams()) {
		for (const Type *type : diagram->types()) {
			if (dynamic_cast<const GraphicType *>(type)) {
				const QString elementType = NameNormalizer::normalize(type->qualifiedName());
				out() << "\taddNode(*(new " << elementType << "(*this)));\n";
			}
		}
	}

	out() << "\n\t// Adding generalizations...\n";

	for (const Diagram *diagram : mEditors[mCurrentEditor]->diagrams()) {
		for (const Type *type : diagram->types()) {
			if (const GraphicType *graphicType = dynamic_cast<const GraphicType *>(type)) {
				for (const QString &parent: graphicType->immediateParents()) {
					const Type *parentType = mEditors[mCurrentEditor]->findType(parent);
					if (!parentType || !mEditors[mCurrentEditor]->diagrams().contains(parentType->diagram()->name())) {
						// Ignoring imported types.
						continue;
					}

					out() << QString("\tproduceEdge(elementType(\"%1\", \"%2\"), elementType(\"%3\", \"%4\"), %5);\n")
							.arg(diagram->name(), type->name(), diagram->name(), parent
									, QString::number(Type::generalizationLinkType));
				}
			}
		}
	}

	out() << "}\n\n";
}

void XmlCompiler::generateNameMappings(OutFile &out)
{
	out() << "void " << mPluginName << "Plugin::initNameMap()\n{\n";

	for (Diagram *diagram : mEditors[mCurrentEditor]->diagrams().values()) {
		const QString diagramName = NameNormalizer::normalize(diagram->name());
		const QString nodeName = NameNormalizer::normalize(diagram->nodeName());
		out() << "\tmDiagramNameMap[\"" << diagramName << "\"] = QObject::tr(\""
				<< diagram->displayedName() << "\");\n";
		out() << "\tmDiagramNodeNameMap[\"" << diagramName << "\"] = \"" << nodeName << "\"" << ";\n";
		out() << "\n";
	}

	out() << "}\n\n";
}

void XmlCompiler::generatePaletteGroupsLists(utils::OutFile &out)
{
	out() << "void " << mPluginName << "Plugin::initPaletteGroupsMap()\n{\n";

	for (Diagram *diagram : mEditors[mCurrentEditor]->diagrams().values()) {
		QString diagramName = NameNormalizer::normalize(diagram->name());
		QList<QPair<QString, QStringList>> paletteGroups = diagram->paletteGroups();
		for (QPair<QString, QStringList> const &group: paletteGroups) {
			const QString groupName = group.first;

			out() << "\t{\n";
			out() << "\t\tQStringList groupElements;\n";

			for (const QString &name : group.second) {
				out() << "\t\tgroupElements << QString::fromUtf8(\"" << NameNormalizer::normalize(name) << "\");\n";
			}

			out() << "\t\tmPaletteGroupsMap[QString::fromUtf8(\""
				<< diagramName << "\")].append(qMakePair(QObject::tr(\""
				<< groupName << "\"), groupElements));\n";

			out() << "\t}\n";
		}
	}
	out() << "}\n\n";
}

void XmlCompiler::generatePaletteGroupsDescriptions(utils::OutFile &out)
{
	out() << "void " << mPluginName << "Plugin::initPaletteGroupsDescriptionMap()\n{\n";

	foreach (Diagram *diagram, mEditors[mCurrentEditor]->diagrams().values()) {
		QString diagramName = NameNormalizer::normalize(diagram->name());
		QMap<QString, QString> paletteGroupsDescriptions = diagram->paletteGroupsDescriptions();
		foreach (const QString &groupName, paletteGroupsDescriptions.keys()) {
			const QString descriptionName = paletteGroupsDescriptions[groupName];
			if (!descriptionName.isEmpty()) {
				out() << "\tmPaletteGroupsDescriptionMap[QString::fromUtf8(\""
					<< diagramName << "\")][QObject::tr(\""
					<< groupName << "\")] = QObject::tr(\""
					<< descriptionName << "\");\n";
			}
		}
	}
	out() << "}\n\n";
}

void XmlCompiler::generateShallPaletteBeSorted(utils::OutFile &out)
{
	out() << "void " << mPluginName << "Plugin::initShallPaletteBeSortedMap()\n{\n";

	for (const Diagram * const diagram : mEditors[mCurrentEditor]->diagrams().values()) {
		const QString diagramName = NameNormalizer::normalize(diagram->name());
		out() << "\tmShallPaletteBeSortedMap[QString::fromUtf8(\""
			<< diagramName << "\")] = " << (diagram->shallPaletteBeSorted() ? "true" : "false") << ";\n";
	}
	out() << "}\n\n";
}

void XmlCompiler::generateExplosionsMappings(OutFile &out)
{
	out() << "void " << mPluginName << "Plugin::initExplosionsMap()\n{\n";
	for (Diagram *diagram : mEditors[mCurrentEditor]->diagrams().values()) {
		for (const Type *type : diagram->types().values()) {
			if (const GraphicType *graphicType = dynamic_cast<const GraphicType *>(type)) {
				graphicType->generateExplosionsMap(out);
			}
		}
	}

	out() << "}\n\n";
}

void XmlCompiler::generateNameMappingsRequests(OutFile &out)
{
	out() << "QStringList " << mPluginName << "Plugin::diagrams() const\n{\n"
		<< "\treturn mDiagramNameMap.keys();\n"
		<< "}\n\n"

		<< "QStringList " << mPluginName << "Plugin::diagramPaletteGroups(const QString &diagram) const\n{\n"

		<< "\tQStringList result;\n"
		<< "\tfor (QPair<QString, QStringList> const &group : mPaletteGroupsMap[diagram]) {\n"
		<< "\t\tresult << group.first;\n"
		<< "\t}\n"
		<< "\n"
		<< "\treturn result;\n"
		<< "}\n\n"

		<< "QStringList " << mPluginName << "Plugin::diagramPaletteGroupList(const QString &diagram, "
		<< "const QString &group) const\n{\n"
		<< "\tfor (QPair<QString, QStringList> const &ourGroup : mPaletteGroupsMap[diagram]) {\n"
		<< "\t\tif (ourGroup.first == group) {\n"
		<< "\t\t\treturn ourGroup.second;\n"
		<< "\t\t}\n"
		<< "\t}\n"
		<< "\n"
		<< "\treturn QStringList();\n"
		<< "}\n\n"

		<< "QString " << mPluginName << "Plugin::diagramPaletteGroupDescription(const QString &diagram, "
		<< "const QString &group) const\n{\n"
		<< "\treturn mPaletteGroupsDescriptionMap[diagram][group];\n"
		<< "}\n\n"

		<< "bool " << mPluginName << "Plugin::shallPaletteBeSorted(const QString &diagram) const\n{\n"
		<< "\treturn mShallPaletteBeSortedMap[diagram];\n"
		<< "}\n\n"

		<< "QString " << mPluginName << "Plugin::editorName() const\n{\n"
		<< "\t return \"" << mPluginName << " Editor\";\n"
		<< "}\n\n"

		<< "QString " << mPluginName << "Plugin::diagramName(const QString &diagram) const\n{\n"
		<< "\treturn mDiagramNameMap[diagram];\n"
		<< "}\n\n"

		<< "qReal::ElementType *" << mPluginName << "Plugin::diagramNode(const QString &diagram) const\n{\n"
		<< "\treturn mDiagramNodeNameMap[diagram].isEmpty() ? nullptr "
				": &elementType(diagram, mDiagramNodeNameMap[diagram]);\n"
		<< "}\n\n"

		<< "QList<qReal::Metamodel::ExplosionData>" << mPluginName
				<< "Plugin::explosions(const QString &diagram, "
		<< "const QString &element) const \n{\n"
		<< "\treturn mExplosionsMap[diagram][element];\n"
		<< "}\n\n";
}

// Набор классов ниже представляет собой эмуляцию средствами C++ лямбда-выражений.
// Цель: разделить действие, выполняемое при хитром обходе хитрой структуры данных
// и саму логику хитрого обхода. В нормальных языках хитрый обход можно сделать методом,
// принимающим функцию, вызываемую при посещении каждого элемента. В C++ так тоже
// можно, но невежливо, поэтому делается так: тоже есть метод, реализующий
// хитрый обход, ему вместо функции передаётся объект-действие, реализующий некий
// интерфейс. Интерфейс содержит только один метод, который принимает в качестве параметров
// элемент структуры, который мы хотим посетить, некоторые дополнительные параметры,
// говорящии о состоянии обхода, и некоторые параметры из внешнего контекста
// (для которых в нормальных языках вообще есть замыкания).
// Здесь: обход (не очень хитрый) - это generateListMethod, интерфейс -
// ListMethodGenerator, объекты-действия - PropertiesGenerator и т.д.
// Примечание: на С++ это выглядит уродски, на C# вообще лишнего кода бы не было.
// Даже в Java с анонимными классами это бы выглядело лучше.
class XmlCompiler::ListMethodGenerator
{
public:
	virtual ~ListMethodGenerator() {}
	virtual bool generate(Type *type, OutFile &out, bool isNotFirst) const = 0;
};

class XmlCompiler::ContainedTypesGenerator: public XmlCompiler::ListMethodGenerator
{
public:
	virtual bool generate(Type *type, OutFile &out, bool isNotFirst) const {
		if (const GraphicType *graphicType = dynamic_cast<const GraphicType *>(type)) {
			return graphicType->generateContainedTypes(out, isNotFirst);
		}

		return false;
	}
};

class XmlCompiler::PossibleEdgesGenerator: public XmlCompiler::ListMethodGenerator
{
public:
	virtual bool generate(Type *type, OutFile &out, bool isNotFirst) const {
		if (const GraphicType *graphicType = dynamic_cast<const GraphicType *>(type)) {
			return graphicType->generatePossibleEdges(out, isNotFirst);
		}

		return false;
	}
};

class XmlCompiler::EnumValuesGenerator: public XmlCompiler::ListMethodGenerator
{
public:
	virtual bool generate(Type *type, OutFile &out, bool isNotFirst) const {
		if (const EnumType *enumType = dynamic_cast<const EnumType *>(type)) {
			return enumType->generateValues(out, isNotFirst);
		}

		return false;
	}
};

void XmlCompiler::generateListMethod(OutFile &out, const QString &signature, const ListMethodGenerator &generator)
{
	out() << "QStringList " << mPluginName << "Plugin::" << signature << " const\n"
		<< "{\n"
		<< "\tQStringList result;\n";

	bool isNotFirst = false;

	foreach (Diagram *diagram, mEditors[mCurrentEditor]->diagrams().values())
		foreach (Type *type, diagram->types().values())
			isNotFirst |= generator.generate(type, out, isNotFirst);

	if (!isNotFirst)
		out() << "\tQ_UNUSED(element);\n";
	out() << "\treturn result;\n"
		  << "}\n\n";
}

void XmlCompiler::generateStringSet(OutFile &out, const QSet<QString> &set) const
{
	out() << "QSet<QString>({\n";
	for (const QString &element : set) {
		out() << "\t\t\"" << element << "\",\n";
	}

	out() << "\t});\n";
}

void XmlCompiler::generateStringList(OutFile &out, const QStringList &list) const
{
	out() << "{";
	for (const QString &string : list) {
		out() << "\"" << string << "\", ";
	}

	out() << "}";
}

void XmlCompiler::generateStringListMap(OutFile &out, const QMap<QString, QStringList> &map) const
{
	out() << "QMap<QString, QStringList>({\n";
	for (const QString &key : map.keys()) {
		out() << "\t\t{" << StringUtils::wrap(key) << ", ";
		generateStringList(out, map[key]);
		out() << "},\n";
	}

	out() << "\t})";
}

void XmlCompiler::generatePossibleEdges(utils::OutFile &out)
{
	PossibleEdgesGenerator generator;
		out() << "QList<QPair<QPair<QString,QString>,QPair<bool,QString> > > " << mPluginName
			<< "Plugin::getPossibleEdges(const QString &element) const\n"
			<< "{\n"
			<< "\tQList<QPair<QPair<QString,QString>,QPair<bool,QString> > > result;\n";
	bool isNotFirst = false;

	foreach (Diagram *diagram, mEditors[mCurrentEditor]->diagrams().values())
		foreach (Type *type, diagram->types().values())
			isNotFirst |= generator.generate(type, out, isNotFirst);

	if (!isNotFirst)
		out() << "\tQ_UNUSED(element);\n";
		out() << "\treturn result;\n"
		<< "}\n\n";
}

void XmlCompiler::generateContainedTypes(OutFile &out)
{
	generateListMethod(out, "getTypesContainedBy(const QString &element)", ContainedTypesGenerator());
}

void XmlCompiler::generateResourceFile()
{
	OutFile out("plugin.qrc");
	out() << mResources
		<< "</qresource>\n"
		<< "</RCC>\n";
}

void XmlCompiler::generateEnumValues(OutFile &out)
{
	out() << "QList<QPair<QString, QString>> " << mPluginName
			<< "Plugin::enumValues(const QString &name) const \n{\n";

	EnumValuesGenerator generator;
	bool isNotFirst = false;

	foreach (EnumType *type, mEditors[mCurrentEditor]->getAllEnumTypes())
		isNotFirst |= generator.generate(type, out, isNotFirst);

	if (!isNotFirst) {
		out() << "\tQ_UNUSED(name);\n";
	}

	out() << "\treturn {};\n"
		<< "}\n\n";
}

void XmlCompiler::generateEditableEnums(OutFile &out)
{
	out() << "bool " << mPluginName << "Plugin::isEnumEditable(const QString &name) const\n{\n";

	QStringList editableEnums;
	for (const EnumType *type : mEditors[mCurrentEditor]->getAllEnumTypes()) {
		if (type->isEditable()) {
			editableEnums << StringUtils::wrap(type->name());
		}
	}

	out() << QString("\tconst QStringList editableEnums = { %1 };\n").arg(editableEnums.join(", "));

	out() << "\treturn editableEnums.contains(name);\n"
		<< "}\n\n";
}

/* Copyright 2007-2015 QReal Research Group
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

#include "forkCallGenerator.h"

#include <qrutils/nameNormalizer.h>

using namespace generatorBase::simple;

ForkCallGenerator::ForkCallGenerator(const qrRepo::RepoApi &repo
		, GeneratorCustomizer &customizer
		, const qReal::Id &id
		, const QMap<qReal::Id, QString> &threads
		, QObject *parent)
	: AbstractSimpleGenerator(repo, customizer, id, parent)
	, mThreads(threads)
{
}

QString ForkCallGenerator::generate()
{
	QString result;
	const QString callPattern = readTemplate("threads/call.t");
	for (const qReal::Id &thread : mThreads.keys()) {
		const QString threadName = utils::NameNormalizer::normalizeStrongly(thread.id(), false);
		const QString threadId = mThreads[thread];
		result += QString(callPattern).replace("@@THREAD_ID@@", threadId).replace("@@NAME@@", threadName);
	}

	return result;
}

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

#include "systemGenerator.h"

#include <qrutils/stringUtils.h>
#include <generatorBase/generatorCustomizer.h>

using namespace stm32::simple;
using namespace generatorBase::simple;

SystemGenerator::SystemGenerator(const qrRepo::RepoApi &repo
		, generatorBase::GeneratorCustomizer &customizer
		, const qReal::Id &id
		, QObject *parent)
	: BindingGenerator(repo, customizer, id
			, repo.property(id, "Code").toBool() ? "nativeCode.t" : "system.t"
			, { repo.property(id, "Code").toBool()
					? Binding::createStatic("@@COMMAND@@"
							, utils::StringUtils::dequote(repo.property(id, "Command").toString()))
					: repo.property(id, "Evaluate").toBool()
							? Binding::createConverting("@@COMMAND@@", "Command"
									, customizer.factory()->stringPropertyConverter(id, "Command"))
							: Binding::createStatic("@@COMMAND@@"
									, utils::StringUtils::wrap(repo.property(id, "Command").toString()))
			}
			, parent)
{
}

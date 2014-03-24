#pragma once

#include "nonZoneNode.h"

namespace qReal {
namespace robots {
namespace generators {
namespace semantics {

/// A semantic node for the block with final semantics
class FinalNode : public NonZoneNode
{
public:
	/// @param inMainDiagram Is corresponding final node placed on the main diagram.
	/// It is important to differentiate theese cases because generated code for
	/// the main diagram final block may differ from subprograms one
	/// (for example, terminating task vs 'return')
	FinalNode(Id const &idBinded, bool inMainDigram, QObject *parent = 0);

protected:
	QLinkedList<SemanticNode *> children() const override;
	QString toStringImpl(GeneratorCustomizer &customizer, int indent) const override;

private:
	bool const mInMainDiagram;
};

}
}
}
}

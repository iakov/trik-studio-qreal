#pragma once

#include <interpreterBase/robotModel/portInfo.h>
#include <interpreterBase/robotModel/robotParts/motor.h>
#include <utils/robotCommunication/robotCommunicator.h>

namespace trikKitInterpreter {
namespace robotModel {
namespace real {
namespace parts {

class Motor : public interpreterBase::robotModel::robotParts::Motor
{
	Q_OBJECT

public:
	Motor(interpreterBase::robotModel::DeviceInfo const &info
			, interpreterBase::robotModel::PortInfo const &port);

	/// @todo: break mode makes sense only for nxt

	void on(int speed, bool breakMode = true) override;
	void on(int speed, long unsigned int degrees, bool breakMode = true) override;
	void stop(bool breakMode = true) override;
	void off() override;

protected:
	void doConfiguration() override;
};

}
}
}
}

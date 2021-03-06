#pragma once
#include <ostream>
#include <mutex>
#include <utility>
#include <vector>
#include <string_view>
#include <memory>
#include <array>
#include "type.h"

using namespace std::string_view_literals;

enum class loglevel {
    fatal,
    error,
    warning,
    info,
    verbose,
    trace
};

namespace sqf {
    class instruction;
    namespace parse {
		struct astnode;
		class preprocessorfileinfo;
		class position_info;
    }
}

class LogLocationInfo {
public:
    LogLocationInfo() = default;
    LogLocationInfo(const sqf::parse::preprocessorfileinfo&);
    LogLocationInfo(const sqf::parse::astnode&);
    LogLocationInfo(const sqf::parse::position_info&);
    LogLocationInfo(const sqf::instruction&);

    std::string path;
    size_t line;
    size_t col;

    [[nodiscard]] std::string format() const;
};


class LogMessageBase {
public:
    LogMessageBase(loglevel level, size_t code) : level(level), errorCode(code) {}
    virtual ~LogMessageBase() = default;

    [[nodiscard]] virtual std::string formatMessage() const = 0;
    virtual loglevel getLevel() {
        return level;
    }
    virtual size_t getErrorCode() {
        return errorCode;
    }
    operator LogMessageBase*(){
        return this;
    }
protected:
    loglevel level = loglevel::verbose;
    size_t errorCode;
};
class Logger {
protected:
	std::vector<bool> enabledWarningLevels;
public:
	Logger() : enabledWarningLevels({ true, true, true, true, true, true }) {}
	[[nodiscard]] bool isEnabled(loglevel level) const {
		return enabledWarningLevels[static_cast<size_t>(level)];
	}
	void setEnabled(loglevel level, bool isEnabled) {
		enabledWarningLevels[static_cast<size_t>(level)] = isEnabled;
	}
	virtual void log(loglevel, std::string_view message) = 0;
	static std::string_view loglevelstring(loglevel level)
	{
		switch (level) {
		case loglevel::fatal: return "[FAT]"sv;
		case loglevel::error: return "[ERR]"sv;
		case loglevel::warning: return "[WRN]"sv;
		case loglevel::info: return "[INF]"sv;
		case loglevel::verbose: return "[VBS]"sv;
		case loglevel::trace: return "[TRC]"sv;
		default: return "[???]"sv;
		}
	}
};
class StdOutLogger : public Logger {
public:
	StdOutLogger() : Logger() {}

	virtual void log(loglevel, std::string_view message) override;
};

//Classes that can log, inherit from this
class CanLog {
    Logger& m_logger;
protected:
	Logger& get_logger() { return m_logger; }
    void log(LogMessageBase& message) const;
    void log(LogMessageBase&& message) const;
public:
    CanLog(Logger& logger) : m_logger(logger) {}
};


namespace logmessage {

    namespace preprocessor {
        class PreprocBase : public LogMessageBase {
        public:
            PreprocBase(loglevel level, size_t errorCode, LogLocationInfo location) : 
            LogMessageBase(level, errorCode), location(std::move(location)) {}
        protected:
            LogLocationInfo location;
        };

        class ArgCountMissmatch : public PreprocBase {
            static const loglevel level = loglevel::error;
			static const size_t errorCode = 10001;
        public:
            ArgCountMissmatch(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class UnexpectedDataAfterInclude : public PreprocBase {
            static const loglevel level = loglevel::warning;
            static const size_t errorCode = 10002;
        public:
            UnexpectedDataAfterInclude(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class RecursiveInclude : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10003;
            std::string includeTree;
        public:
            RecursiveInclude(LogLocationInfo loc, std::string includeTree) :
                PreprocBase(level, errorCode, std::move(loc)), includeTree(std::move(includeTree)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class IncludeFailed : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10004;
            std::string_view line;
            const std::runtime_error& exception;
        public:
            IncludeFailed(LogLocationInfo loc, std::string_view line, const std::runtime_error& exception) :
                PreprocBase(level, errorCode, std::move(loc)), line(line), exception(exception) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class MacroDefinedTwice : public PreprocBase {
            static const loglevel level = loglevel::warning;
            static const size_t errorCode = 10005;
            std::string_view macroname;
        public:
            MacroDefinedTwice(LogLocationInfo loc, std::string_view macroname) :
                PreprocBase(level, errorCode, std::move(loc)), macroname(macroname) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class MacroNotFound : public PreprocBase {
            static const loglevel level = loglevel::warning;
            static const size_t errorCode = 10006;
            std::string_view macroname;
        public:
            MacroNotFound(LogLocationInfo loc, std::string_view macroname) :
                PreprocBase(level, errorCode, std::move(loc)), macroname(macroname) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class UnexpectedIfdef : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10007;
        public:
            UnexpectedIfdef(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class UnexpectedIfndef : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10008;
        public:
            UnexpectedIfndef(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class UnexpectedElse : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10009;
        public:
            UnexpectedElse(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

		class UnexpectedEndif : public PreprocBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 10010;
		public:
			UnexpectedEndif(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class MissingEndif : public PreprocBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 10011;
		public:
			MissingEndif(LogLocationInfo loc) : PreprocBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};

        class UnknownInstruction : public PreprocBase {
            static const loglevel level = loglevel::error;
            static const size_t errorCode = 10012;
            std::string_view instruction;
        public:
            UnknownInstruction(LogLocationInfo loc, std::string_view instruction) :
                PreprocBase(level, errorCode, std::move(loc)), instruction(instruction) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

        class EmptyArgument : public PreprocBase {
            static const loglevel level = loglevel::warning;
            static const size_t errorCode = 10013;
        public:
			EmptyArgument(LogLocationInfo loc) :
                PreprocBase(level, errorCode, std::move(loc)) {}
            [[nodiscard]] std::string formatMessage() const override;
        };

    }
	namespace assembly
	{
		class AssemblyBase : public LogMessageBase {
		public:
			AssemblyBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};

		class ExpectedSemicolon : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20001;
		public:
			ExpectedSemicolon(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeInstructions : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20002;
		public:
			NoViableAlternativeInstructions(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeArg : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20003;
		public:
			NoViableAlternativeArg(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedEndStatement : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20004;
		public:
			ExpectedEndStatement(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedCallNular : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20005;
		public:
			ExpectedCallNular(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedNularOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20006;
		public:
			ExpectedNularOperator(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class UnknownNularOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20007;
			std::string_view operator_name;
		public:
			UnknownNularOperator(LogLocationInfo loc, std::string_view operator_name) :
				AssemblyBase(level, errorCode, std::move(loc)), operator_name(operator_name) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedCallUnary : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20008;
		public:
			ExpectedCallUnary(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedUnaryOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20009;
		public:
			ExpectedUnaryOperator(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class UnknownUnaryOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20010;
			std::string_view operator_name;
		public:
			UnknownUnaryOperator(LogLocationInfo loc, std::string_view operator_name) :
				AssemblyBase(level, errorCode, std::move(loc)), operator_name(operator_name) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedCallBinary : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20011;
		public:
			ExpectedCallBinary(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedBinaryOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20012;
		public:
			ExpectedBinaryOperator(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class UnknownBinaryOperator : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20013;
			std::string_view operator_name;
		public:
			UnknownBinaryOperator(LogLocationInfo loc, std::string_view operator_name) :
				AssemblyBase(level, errorCode, std::move(loc)), operator_name(operator_name) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedAssignTo : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20014;
		public:
			ExpectedAssignTo(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedVariableName : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20015;
		public:
			ExpectedVariableName(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedAssignToLocal : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20016;
		public:
			ExpectedAssignToLocal(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedGetVariable : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20017;
		public:
			ExpectedGetVariable(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedMakeArray : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20018;
		public:
			ExpectedMakeArray(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedInteger : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20019;
		public:
			ExpectedInteger(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedPush : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20020;
		public:
			ExpectedPush(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedTypeName : public AssemblyBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 20021;
		public:
			ExpectedTypeName(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NumberOutOfRange : public AssemblyBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 20022;
		public:
			NumberOutOfRange(LogLocationInfo loc) : AssemblyBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
	namespace sqf
	{
		class SqfBase : public LogMessageBase {
		public:
			SqfBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};

		class ExpectedStatementTerminator : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30001;
		public:
			ExpectedStatementTerminator(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeStatement : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30002;
		public:
			NoViableAlternativeStatement(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingUnderscoreOnPrivateVariable : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30003;
			std::string_view m_variable_name;
		public:
			MissingUnderscoreOnPrivateVariable(LogLocationInfo loc, std::string_view variable_name) :
				SqfBase(level, errorCode, std::move(loc)),
			m_variable_name(variable_name) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedBinaryExpression : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30004;
		public:
			ExpectedBinaryExpression(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingRightArgument : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30005;
			std::string_view m_operator_name;
		public:
			MissingRightArgument(LogLocationInfo loc, std::string_view operator_name) :
				SqfBase(level, errorCode, std::move(loc)),
				m_operator_name(operator_name) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingRoundClosingBracket : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30006;
		public:
			MissingRoundClosingBracket(LogLocationInfo loc) :
				SqfBase(level, errorCode, std::move(loc)){ }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingCurlyClosingBracket : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30007;
		public:
			MissingCurlyClosingBracket(LogLocationInfo loc) :
				SqfBase(level, errorCode, std::move(loc)){ }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingSquareClosingBracket : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30008;
		public:
			MissingSquareClosingBracket(LogLocationInfo loc) :
				SqfBase(level, errorCode, std::move(loc)){ }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativePrimaryExpression : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30009;
		public:
			NoViableAlternativePrimaryExpression(LogLocationInfo loc) :
				SqfBase(level, errorCode, std::move(loc)){ }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class EmptyNumber : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30010;
		public:
			EmptyNumber(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedSQF : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30011;
		public:
			ExpectedSQF(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class EndOfFile : public SqfBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 30012;
		public:
			EndOfFile(LogLocationInfo loc) : SqfBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
	namespace config
	{
		class ConfigBase : public LogMessageBase {
		public:
			ConfigBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};
		class ExpectedStatementTerminator : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40001;
		public:
			ExpectedStatementTerminator(LogLocationInfo loc) : ConfigBase(level, errorCode, std::move(loc)) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeNode : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40002;
		public:
			NoViableAlternativeNode(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedIdentifier : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40003;
		public:
			ExpectedIdentifier(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingRoundClosingBracket : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40004;
		public:
			MissingRoundClosingBracket(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingCurlyOpeningBracket : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40005;
		public:
			MissingCurlyOpeningBracket(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingCurlyClosingBracket : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40006;
		public:
			MissingCurlyClosingBracket(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingSquareClosingBracket : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40007;
		public:
			MissingSquareClosingBracket(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MissingEqualSign : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40008;
		public:
			MissingEqualSign(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArray : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40009;
		public:
			ExpectedArray(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedValue : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40010;
		public:
			ExpectedValue(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoViableAlternativeValue : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40011;
		public:
			NoViableAlternativeValue(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
		class EndOfFileNotReached : public ConfigBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 40012;
		public:
			EndOfFileNotReached(LogLocationInfo loc) :
				ConfigBase(level, errorCode, std::move(loc)) { }
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
	namespace linting
	{
		class LintingBase : public LogMessageBase {
		public:
			LintingBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};
		class UnassignedVariable : public LintingBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 50001;
			std::string_view m_variable_name;
		public:
			UnassignedVariable(LogLocationInfo loc, std::string_view variable_name) :
				LintingBase(level, errorCode, std::move(loc)),
				m_variable_name(variable_name) {}
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
	namespace runtime
	{
		class RuntimeBase : public LogMessageBase {
		public:
			RuntimeBase(loglevel level, size_t errorCode, LogLocationInfo location) :
				LogMessageBase(level, errorCode), location(std::move(location)) {}
		protected:
			LogLocationInfo location;
		};
		class Stacktrace : public RuntimeBase {
			static const loglevel level = loglevel::fatal;
			static const size_t errorCode = 60001;
			std::string m_stacktrace;
		public:
			Stacktrace(LogLocationInfo loc, std::string stacktrace) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_stacktrace(stacktrace)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MaximumInstructionCountReached : public RuntimeBase {
			static const loglevel level = loglevel::fatal;
			static const size_t errorCode = 60002;
			size_t m_maximum_instruction_count;
		public:
			MaximumInstructionCountReached(LogLocationInfo loc, size_t maximum_instruction_count) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_maximum_instruction_count(maximum_instruction_count)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArraySizeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60003;
			size_t m_expected_min;
			size_t m_expected_max;
			size_t m_got;
		public:
			ExpectedArraySizeMissmatch(LogLocationInfo loc, size_t expected, size_t got) :
				ExpectedArraySizeMissmatch(loc, expected, expected, got)
			{}
			ExpectedArraySizeMissmatch(LogLocationInfo loc, size_t expected_min, size_t expected_max, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected_min(expected_min),
				m_expected_max(expected_max),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArraySizeMissmatchWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60004;
			size_t m_expected_min;
			size_t m_expected_max;
			size_t m_got;
		public:
			ExpectedArraySizeMissmatchWeak(LogLocationInfo loc, size_t expected, size_t got) :
				ExpectedArraySizeMissmatchWeak(loc, expected, expected, got)
			{}
			ExpectedArraySizeMissmatchWeak(LogLocationInfo loc, size_t expected_min, size_t expected_max, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected_min(expected_min),
				m_expected_max(expected_max),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedMinimumArraySizeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60005;
			size_t m_expected;
			size_t m_got;
		public:
			ExpectedMinimumArraySizeMissmatch(LogLocationInfo loc, size_t expected, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedMinimumArraySizeMissmatchWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60006;
			size_t m_expected;
			size_t m_got;
		public:
			ExpectedMinimumArraySizeMissmatchWeak(LogLocationInfo loc, size_t expected, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArrayTypeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60007;
			size_t m_position;
			std::vector<::sqf::type> m_expected;
			::sqf::type m_got;
		public:
			ExpectedArrayTypeMissmatch(LogLocationInfo loc, size_t position, std::vector<::sqf::type> expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_position(position),
				m_expected(expected),
				m_got(got)
			{}
			ExpectedArrayTypeMissmatch(LogLocationInfo loc, size_t position, ::sqf::type expected, ::sqf::type got) :
				ExpectedArrayTypeMissmatch(loc, position, std::array<::sqf::type, 1> { expected }, got)
			{}
			template<size_t size>
			ExpectedArrayTypeMissmatch(LogLocationInfo loc, size_t position, std::array<::sqf::type, size> expected, ::sqf::type got) :
				ExpectedArrayTypeMissmatch(loc, position, std::vector<::sqf::type>(expected.begin(), expected.end()), got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArrayTypeMissmatchWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60008;
			size_t m_position;
			std::vector<::sqf::type> m_expected;
			::sqf::type m_got;
		public:
			ExpectedArrayTypeMissmatchWeak(LogLocationInfo loc, size_t position, std::vector<::sqf::type> expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_position(position),
				m_expected(expected),
				m_got(got)
			{}
			ExpectedArrayTypeMissmatchWeak(LogLocationInfo loc, size_t position, ::sqf::type expected, ::sqf::type got) :
				ExpectedArrayTypeMissmatchWeak(loc, position, std::array<::sqf::type, 1> { expected }, got)
			{}
			template<size_t size>
			ExpectedArrayTypeMissmatchWeak(LogLocationInfo loc, size_t position, std::array<::sqf::type, size> expected, ::sqf::type got) :
				ExpectedArrayTypeMissmatchWeak(loc, position, std::vector<::sqf::type>(expected.begin(), expected.end()), got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class IndexOutOfRange : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60009;
			size_t m_range;
			size_t m_index;
		public:
			IndexOutOfRange(LogLocationInfo loc, size_t range, size_t index) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_range(range),
				m_index(index)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class IndexOutOfRangeWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60010;
			size_t m_range;
			size_t m_index;
		public:
			IndexOutOfRangeWeak(LogLocationInfo loc, size_t range, size_t index) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_range(range),
				m_index(index)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class NegativeIndex : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60011;
		public:
			NegativeIndex(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NegativeIndexWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60012;
		public:
			NegativeIndexWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class IndexEqualsRange : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60013;
			size_t m_range;
			size_t m_index;
		public:
			IndexEqualsRange(LogLocationInfo loc, size_t range, size_t index) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_range(range),
				m_index(index)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningNil : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60014;
		public:
			ReturningNil(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningEmptyArray : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60015;
		public:
			ReturningEmptyArray(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NegativeSize : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60016;
		public:
			NegativeSize(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NegativeSizeWeak: public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60017;
		public:
			NegativeSizeWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ArrayRecursion: public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60018;
		public:
			ArrayRecursion(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class InfoMessage: public RuntimeBase {
			static const loglevel level = loglevel::info;
			static const size_t errorCode = 60019;
			std::string_view m_source;
			std::string m_message;
		public:
			InfoMessage(LogLocationInfo loc, std::string_view source, std::string message) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_source(source),
				m_message(message)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class SuspensionDisabled : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60020;
		public:
			SuspensionDisabled(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class SuspensionInUnscheduledEnvironment : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60021;
		public:
			SuspensionInUnscheduledEnvironment(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningConfigNull : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60022;
		public:
			ReturningConfigNull(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class AssertFailed : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60023;
		public:
			AssertFailed(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class StartIndexExceedsToIndex : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60024;
			size_t m_from;
			size_t m_to;
		public:
			StartIndexExceedsToIndex(LogLocationInfo loc, size_t from, size_t to) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_from(from),
				m_to(to)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class StartIndexExceedsToIndexWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60025;
			size_t m_from;
			size_t m_to;
		public:
			StartIndexExceedsToIndexWeak(LogLocationInfo loc, size_t from, size_t to) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_from(from),
				m_to(to)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MagicVariableTypeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60026;
			std::string_view m_variable_name;
			::sqf::type m_expected;
			::sqf::type m_got;
		public:
			MagicVariableTypeMissmatch(LogLocationInfo loc, std::string_view variable_name, ::sqf::type expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_variable_name(variable_name),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ScriptHandleAlreadyTerminated : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60027;
		public:
			ScriptHandleAlreadyTerminated(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ScriptHandleAlreadyFinished : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60028;
		public:
			ScriptHandleAlreadyFinished(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExtensionLoaded : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60029;
			std::string_view m_extension_name;
			std::string m_version;
		public:
			ExtensionLoaded(LogLocationInfo loc, std::string_view extension_name, std::string version) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_extension_name(extension_name),
				m_version(version)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExtensionNotTerminatingVersionString : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60030;
			std::string_view m_extension_name;
		public:
			ExtensionNotTerminatingVersionString(LogLocationInfo loc, std::string_view extension_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_extension_name(extension_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class ExtensionNotTerminatingCallExtensionBufferString : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60031;
			std::string_view m_extension_name;
		public:
			ExtensionNotTerminatingCallExtensionBufferString(LogLocationInfo loc, std::string_view extension_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_extension_name(extension_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExtensionNotTerminatingCallExtensionArgBufferString : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60032;
			std::string_view m_extension_name;
		public:
			ExtensionNotTerminatingCallExtensionArgBufferString(LogLocationInfo loc, std::string_view extension_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_extension_name(extension_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class LibraryNameContainsPath : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60033;
			std::string_view m_extension_name;
		public:
			LibraryNameContainsPath(LogLocationInfo loc, std::string_view extension_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_extension_name(extension_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningEmptyString : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60034;
		public:
			ReturningEmptyString(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExtensionRuntimeError : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60035;
			std::string_view m_extension_name;
			std::string_view m_what;
		public:
			ExtensionRuntimeError(LogLocationInfo loc, std::string_view extension_name, std::string_view what) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_extension_name(extension_name),
				m_what(what)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class FileNotFound : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60036;
			std::string_view m_filename;
		public:
			FileNotFound(LogLocationInfo loc, std::string_view filename) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_filename(filename)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ScopeNameAlreadySet : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60037;
		public:
			ScopeNameAlreadySet(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ScriptNameAlreadySet : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60038;
		public:
			ScriptNameAlreadySet(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningEmptyScriptHandle : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60039;
		public:
			ReturningEmptyScriptHandle(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningErrorCode : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60040;
			std::string_view m_error_code;
		public:
			ReturningErrorCode(LogLocationInfo loc, std::string_view error_code) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_error_code(error_code)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class ExpectedSubArrayTypeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60041;
			std::vector<size_t> m_position;
			std::vector<::sqf::type> m_expected;
			::sqf::type m_got;
		public:
			ExpectedSubArrayTypeMissmatch(LogLocationInfo loc, std::vector<size_t> position, std::vector<::sqf::type> expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_position(position),
				m_expected(expected),
				m_got(got)
			{}
			template<size_t size>
			ExpectedSubArrayTypeMissmatch(LogLocationInfo loc, std::array<size_t, size> position, ::sqf::type expected, ::sqf::type got) :
				ExpectedSubArrayTypeMissmatch(loc, position, std::array<::sqf::type, 1> { expected }, got)
			{}
			template<size_t size1, size_t size2>
			ExpectedSubArrayTypeMissmatch(LogLocationInfo loc, std::array<size_t, size1> position, std::array<::sqf::type, size2> expected, ::sqf::type got) :
				ExpectedSubArrayTypeMissmatch(loc, std::vector<size_t>(position.begin(), position.end()), std::vector<::sqf::type>(expected.begin(), expected.end()), got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedSubArrayTypeMissmatchWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60042;
			std::vector<size_t> m_position;
			std::vector<::sqf::type> m_expected;
			::sqf::type m_got;
		public:
			ExpectedSubArrayTypeMissmatchWeak(LogLocationInfo loc, std::vector<size_t> position, std::vector<::sqf::type> expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_position(position),
				m_expected(expected),
				m_got(got)
			{}
			template<size_t size>
			ExpectedSubArrayTypeMissmatchWeak(LogLocationInfo loc, std::array<size_t, size> position, ::sqf::type expected, ::sqf::type got) :
				ExpectedSubArrayTypeMissmatchWeak(loc, position, std::array<::sqf::type, 1> { expected }, got)
			{}
			template<size_t size1, size_t size2>
			ExpectedSubArrayTypeMissmatchWeak(LogLocationInfo loc, std::array<size_t, size1> position, std::array<::sqf::type, size2> expected, ::sqf::type got) :
				ExpectedSubArrayTypeMissmatchWeak(loc, std::vector<size_t>(position.begin(), position.end()), std::vector<::sqf::type>(expected.begin(), expected.end()), got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ErrorMessage : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60043;
			std::string_view m_source;
			std::string m_message;
		public:
			ErrorMessage(LogLocationInfo loc, std::string_view source, std::string message) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_source(source),
				m_message(message)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class FileSystemDisabled : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60044;
		public:
			FileSystemDisabled(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NetworkingDisabled : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60045;
		public:
			NetworkingDisabled(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class AlreadyConnected : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60046;
		public:
			AlreadyConnected(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NetworkingFormatMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60047;
			std::string_view m_provided;
		public:
			NetworkingFormatMissmatch(LogLocationInfo loc, std::string_view provided) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_provided(provided)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class FailedToEstablishConnection : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60048;
		public:
			FailedToEstablishConnection(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArrayToHaveElements : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60049;
		public:
			ExpectedArrayToHaveElements(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedArrayToHaveElementsWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60050;
		public:
			ExpectedArrayToHaveElementsWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class ClipboardDisabled : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60051;
		public:
			ClipboardDisabled(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class FailedToCopyToClipboard : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60052;
		public:
			FailedToCopyToClipboard(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class FormatInvalidPlaceholder : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60053;
			char m_placeholder;
			size_t m_index;
		public:
			FormatInvalidPlaceholder(LogLocationInfo loc, char placeholder, size_t index) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_placeholder(placeholder),
				m_index(index)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ZeroDivisor : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60054;
		public:
			ZeroDivisor(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MarkerNotExisting : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60055;
			std::string_view m_marker_name;
		public:
			MarkerNotExisting(LogLocationInfo loc, std::string_view marker_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_marker_name(marker_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningDefaultArray : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60056;
			size_t m_size;
		public:
			ReturningDefaultArray(LogLocationInfo loc, size_t size) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_size(size)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningScalarZero : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60057;
		public:
			ReturningScalarZero(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedNonNullValue : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60058;
		public:
			ExpectedNonNullValue(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedNonNullValueWeak: public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60059;
		public:
			ExpectedNonNullValueWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ConfigEntryNotFound : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60060;
			std::vector<std::string> m_config_path;
			std::string m_config_name;
		public:
			ConfigEntryNotFound(LogLocationInfo loc, std::vector<std::string> config_path, std::string config_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_config_path(config_path),
				m_config_name(config_name)
			{}
			template<size_t size>
			ConfigEntryNotFound(LogLocationInfo loc, std::array<std::string, size> config_path, std::string config_name) :
				ConfigEntryNotFound(loc, std::vector<std::string>(config_path.begin(), config_path.end()), config_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class ConfigEntryNotFoundWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60061;
			std::vector<std::string> m_config_path;
			std::string m_config_name;
		public:
			ConfigEntryNotFoundWeak(LogLocationInfo loc, std::vector<std::string> config_path, std::string config_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_config_path(config_path),
				m_config_name(config_name)
			{}
			template<size_t size>
			ConfigEntryNotFoundWeak(LogLocationInfo loc, std::array<std::string, size> config_path, std::string config_name) :
				ConfigEntryNotFoundWeak(loc, std::vector<std::string>(config_path.begin(), config_path.end()), config_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedVehicle : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60062;
		public:
			ExpectedVehicle(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedVehicleWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60063;
		public:
			ExpectedVehicleWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedUnit : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60064;
		public:
			ExpectedUnit(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ExpectedUnitWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60065;
		public:
			ExpectedUnitWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ReturningFalse : public RuntimeBase {
			static const loglevel level = loglevel::verbose;
			static const size_t errorCode = 60066;
		public:
			ReturningFalse(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class MarkerAlreadyExisting : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60067;
			std::string_view m_marker_name;
		public:
			MarkerAlreadyExisting(LogLocationInfo loc, std::string_view marker_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_marker_name(marker_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class InvalidMarkershape : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60067;
			std::string_view m_shape_name;
		public:
			InvalidMarkershape(LogLocationInfo loc, std::string_view shape_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_shape_name(shape_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class TypeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60068;
			::sqf::type m_expected;
			::sqf::type m_got;
		public:
			TypeMissmatch(LogLocationInfo loc, ::sqf::type expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class TypeMissmatchWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60069;
			::sqf::type m_expected;
			::sqf::type m_got;
		public:
			TypeMissmatchWeak(LogLocationInfo loc, ::sqf::type expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class VariableNotFound : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60070;
			std::string_view m_variable_name;
		public:
			VariableNotFound(LogLocationInfo loc, std::string_view variable_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_variable_name(variable_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class StackCorruptionMissingValues : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60071;
			size_t m_expected;
			size_t m_got;
		public:
			StackCorruptionMissingValues(LogLocationInfo loc, size_t expected, size_t got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoValueFoundForRightArgument : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60072;
		public:
			NoValueFoundForRightArgument(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoValueFoundForRightArgumentWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60073;
		public:
			NoValueFoundForRightArgumentWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoValueFoundForLeftArgument : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60074;
		public:
			NoValueFoundForLeftArgument(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class NoValueFoundForLeftArgumentWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60075;
		public:
			NoValueFoundForLeftArgumentWeak(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class UnknownInputTypeCombinationBinary : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60076;
			std::string_view m_operator;
			::sqf::type m_left_got;
			::sqf::type m_right_got;
		public:
			UnknownInputTypeCombinationBinary(LogLocationInfo loc, ::sqf::type left_got, std::string_view op, ::sqf::type right_got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_operator(op),
				m_left_got(left_got),
				m_right_got(right_got)
			{}
			UnknownInputTypeCombinationBinary(LogLocationInfo loc, std::string_view op, ::sqf::type right_got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_operator(op),
				m_left_got(::sqf::type::NA),
				m_right_got(right_got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class FoundNoValue : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60077;
		public:
			FoundNoValue(LogLocationInfo loc) :
				RuntimeBase(level, errorCode, std::move(loc))
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class CallstackFoundNoValue : public RuntimeBase {
			static const loglevel level = loglevel::error;
			static const size_t errorCode = 60078;
			std::string_view m_callstack_name;
		public:
			CallstackFoundNoValue(LogLocationInfo loc, std::string_view callstack_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_callstack_name(callstack_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class CallstackFoundNoValueWeak : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60079;
			std::string_view m_callstack_name;
		public:
			CallstackFoundNoValueWeak(LogLocationInfo loc, std::string_view callstack_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_callstack_name(callstack_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class GroupNotEmpty : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60080;
			std::string_view m_group_name;
		public:
			GroupNotEmpty(LogLocationInfo loc, std::string_view group_name) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_group_name(group_name)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};

		class ForStepVariableTypeMissmatch : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60081;
			std::string_view m_variable_name;
			::sqf::type m_expected;
			::sqf::type m_got;
		public:
			ForStepVariableTypeMissmatch(LogLocationInfo loc, std::string_view variable_name, ::sqf::type expected, ::sqf::type got) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_variable_name(variable_name),
				m_expected(expected),
				m_got(got)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
		class ForStepNoWorkShouldBeDone : public RuntimeBase {
			static const loglevel level = loglevel::warning;
			static const size_t errorCode = 60082;
			double m_step;
			double m_from;
			double m_to;
		public:
			ForStepNoWorkShouldBeDone(LogLocationInfo loc, double step, double from, double to) :
				RuntimeBase(level, errorCode, std::move(loc)),
				m_step(step),
				m_from(from),
				m_to(to)
			{}
			[[nodiscard]] std::string formatMessage() const override;
		};
	}
}


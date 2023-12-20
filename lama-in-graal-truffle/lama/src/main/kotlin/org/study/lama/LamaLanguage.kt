package org.study.lama

import com.oracle.truffle.api.Assumption
import com.oracle.truffle.api.CallTarget
import com.oracle.truffle.api.RootCallTarget
import com.oracle.truffle.api.Truffle
import com.oracle.truffle.api.TruffleLanguage
import com.oracle.truffle.api.dsl.NodeFactory
import com.oracle.truffle.api.frame.FrameDescriptor
import com.oracle.truffle.api.instrumentation.AllocationReporter
import com.oracle.truffle.api.interop.InteropLibrary
import com.oracle.truffle.api.nodes.RootNode
import com.oracle.truffle.api.`object`.Shape
import com.oracle.truffle.api.source.Source
import com.oracle.truffle.api.strings.TruffleString
import java.util.Collections
import java.util.concurrent.ConcurrentHashMap
import org.study.lama.runtime.LamaContext


final class LamaLanguage : TruffleLanguage<LamaContext>() {
    companion object {
        @Volatile var counter: Int = 0

        const val ID = "lama"
        const val MIME_TYPE = "application/x-lama"
        private val BUILTIN_SOURCE: Source = Source.newBuilder(ID, "", "Lama builtin").build()

        val STRING_ENCODING = TruffleString.Encoding.UTF_16
    }

    private val singleContext: Assumption = Truffle.getRuntime().createAssumption("Single Lama context.")

    private val builtinTargets: Map<NodeFactory<out LamaBuiltinNode?>, RootCallTarget> =
        ConcurrentHashMap<NodeFactory<out LamaBuiltinNode?>, RootCallTarget>()
    private val undefinedFunctions: Map<TruffleString, RootCallTarget> = ConcurrentHashMap()

    private val rootShape: Shape? = null

    /**
     * Creates internal representation of the executing context suitable for given environment. Each
     * time the [language][TruffleLanguage] is used by a new
     * [org.graalvm.polyglot.Context], the system calls this method to let the
     * [language][TruffleLanguage] prepare for *execution*. The returned execution
     * context is completely language specific; it is however expected it will contain reference to
     * here-in provided `env` and adjust itself according to parameters provided by the
     * `env` object.
     *
     *
     * The context created by this method is accessible using [context][ContextReference]. An [IllegalStateException] is thrown if the context is tried to be
     * accessed while the createContext method is executed.
     *
     *
     * This method shouldn't perform any complex operations. The runtime system is just being
     * initialized and for example making
     * [calls into][Env.parsePublic] and assuming your language is already initialized and others can see it
     * would be wrong - until you return from this method, the initialization isn't over. The same
     * is true for instrumentation, the instruments cannot receive any meta data about code executed
     * during context creation. Should there be a need to perform complex initialization, do it by
     * overriding the [.initializeContext] method.
     *
     *
     * Additional services provided by the language must be
     * [registered][Env.registerService] by this method otherwise
     * [IllegalStateException] is thrown.
     *
     *
     * May return `null` if the language does not need any per-[context][Context]
     * state. Otherwise it should return a new object instance every time it is called.
     *
     * @param env the environment the language is supposed to operate in
     * @return internal data of the language in given environment or `null`
     * @since 0.8 or earlier
     */
    override fun createContext(env: Env?): LamaContext {
        return LamaContext(this, env, ArrayList<E>(EXTERNAL_BUILTINS))
    }

    init {
        counter++
        this.rootShape = Shape.newBuilder().layout(LamaObject::class.java).build()
    }

    //TODO from this line
    override fun patchContext(context: LamaContext, newEnv: Env?): Boolean {
        context.patchContext(newEnv)
        return true
    }

    fun getOrCreateUndefinedFunction(name: TruffleString?): RootCallTarget {
        var target = undefinedFunctions[name]
        if (target == null) {
            target = LamaUndefinedFunctionRootNode(this, name).getCallTarget()
            val other: RootCallTarget = undefinedFunctions.putIfAbsent(name, target)
            if (other != null) {
                target = other
            }
        }
        return target
    }

    fun lookupBuiltin(factory: NodeFactory<out LamaBuiltinNode?>): RootCallTarget {
        val target = builtinTargets[factory]
        if (target != null) {
            return target
        }

        /*
         * The builtin node factory is a class that is automatically generated by the Truffle DSL.
         * The signature returned by the factory reflects the signature of the @Specialization
         *
         * methods in the builtin classes.
         */
        val argumentCount = factory.executionSignature.size
        val argumentNodes: Array<LamaExpressionNode?> = arrayOfNulls<LamaExpressionNode>(argumentCount)
        /*
         * Builtin functions are like normal functions, i.e., the arguments are passed in as an
         * Object[] array encapsulated in LamaArguments. A LamaReadArgumentNode extracts a parameter
         * from this array.
         */for (i in 0 until argumentCount) {
            argumentNodes[i] = LamaReadArgumentNode(i)
        }
        /* Instantiate the builtin node. This node performs the actual functionality. */
        val builtinBodyNode: LamaBuiltinNode? = factory.createNode(argumentNodes as Any)
        builtinBodyNode.addRootTag()
        /* The name of the builtin function is specified via an annotation on the node class. */
        val name: TruffleString = LamaStrings.fromJavaString(lookupNodeInfo(builtinBodyNode.getClass()).shortName())
        builtinBodyNode.setUnavailableSourceSection()

        /* Wrap the builtin in a RootNode. Truffle requires all AST to start with a RootNode. */
        val rootNode =
            LamaRootNode(this, FrameDescriptor(), builtinBodyNode, BUILTIN_SOURCE.createUnavailableSection(), name)

        /*
         * Register the builtin function in the builtin registry. Call targets for builtins may be
         * reused across multiple contexts.
         */
        val newTarget: RootCallTarget = rootNode.getCallTarget()
        val oldTarget: RootCallTarget = builtinTargets.putIfAbsent(factory, newTarget)
        return oldTarget ?: newTarget
    }

    fun lookupNodeInfo(clazz: Class<*>?): NodeInfo? {
        if (clazz == null) {
            return null
        }
        val info: NodeInfo = clazz.getAnnotation(NodeInfo::class.java)
        return if (info != null) {
            info
        } else {
            lookupNodeInfo(clazz.superclass)
        }
    }

    @Throws(Exception::class)
    override fun parse(request: ParsingRequest): CallTarget {
        val source = request.getSource()
        val functions: Map<TruffleString, RootCallTarget>
        /*
         * Parse the provided source. At this point, we do not have a LamaContext yet. Registration of
         * the functions with the LamaContext happens lazily in LamaEvalRootNode.
         */if (request.getArgumentNames().isEmpty()) {
            functions = LamaParser.parseLama(this, source)
        } else {
            val sb = StringBuilder()
            sb.append("function main(")
            var sep = ""
            for (argumentName in request.getArgumentNames()) {
                sb.append(sep)
                sb.append(argumentName)
                sep = ","
            }
            sb.append(") { return ")
            sb.append(source.characters)
            sb.append(";}")
            val language = if (source.language == null) ID else source.language
            val decoratedSource = Source.newBuilder(language, sb.toString(), source.name).build()
            functions = LamaParser.parseLama(this, decoratedSource)
        }
        val main = functions[LamaStrings.MAIN]
        val evalMain: RootNode
        if (main != null) {
            /*
             * We have a main function, so "evaluating" the parsed source means invoking that main
             * function. However, we need to lazily register functions into the LamaContext first, so
             * we cannot use the original LamaRootNode for the main function. Instead, we create a new
             * LamaEvalRootNode that does everything we need.
             */
            evalMain = LamaEvalRootNode(this, main, functions)
        } else {
            /*
             * Even without a main function, "evaluating" the parsed source needs to register the
             * functions into the LamaContext.
             */
            evalMain = LamaEvalRootNode(this, null, functions)
        }
        return evalMain.getCallTarget()
    }

    /**
     * LamaLanguage specifies the [ContextPolicy.SHARED] in
     * [Registration.contextPolicy]. This means that a single [TruffleLanguage]
     * instance can be reused for multiple language contexts. Before this happens the Truffle
     * framework notifies the language by invoking [.initializeMultipleContexts]. This
     * allows the language to invalidate certain assumptions taken for the single context case. One
     * assumption Lama takes for single context case is located in [LamaEvalRootNode]. There
     * functions are only tried to be registered once in the single context case, but produce a
     * boundary call in the multi context case, as function registration is expected to happen more
     * than once.
     *
     * Value identity caches should be avoided and invalidated for the multiple contexts case as no
     * value will be the same. Instead, in multi context case, a language should only use types,
     * shapes and code to speculate.
     *
     * For a new language it is recommended to start with [ContextPolicy.EXCLUSIVE] and as the
     * language gets more mature switch to [ContextPolicy.SHARED].
     */
    override fun initializeMultipleContexts() {
        singleContext.invalidate()
    }

    fun isSingleContext(): Boolean {
        return singleContext.isValid
    }

    override fun getLanguageView(context: LamaContext?, value: Any?): Any {
        return LamaLanguageView.create(value)
    }

    override fun isVisible(context: LamaContext?, value: Any?): Boolean {
        return !InteropLibrary.getFactory().getUncached(value).isNull(value)
    }

    override fun getScope(context: LamaContext): Any {
        return context.getFunctionRegistry().getFunctionsObject()
    }

    fun getRootShape(): Shape {
        return rootShape!!
    }

    /**
     * Allocate an empty object. All new objects initially have no properties. Properties are added
     * when they are first stored, i.e., the store triggers a shape change of the object.
     */
    fun createObject(reporter: AllocationReporter): LamaObject {
        reporter.onEnter(null, 0, AllocationReporter.SIZE_UNKNOWN)
        val `object` = LamaObject(rootShape)
        reporter.onReturnValue(`object`, 0, AllocationReporter.SIZE_UNKNOWN)
        return `object`
    }

    private val REFERENCE: LanguageReference<LamaLanguage> = LanguageReference.create(LamaLanguage::class.java)

    operator fun get(node: Node?): LamaLanguage {
        return REFERENCE[node]
    }

    private val EXTERNAL_BUILTINS: MutableList<NodeFactory<out LamaBuiltinNode?>> =
        Collections.synchronizedList<NodeFactory<out LamaBuiltinNode?>>(
            ArrayList<NodeFactory<out LamaBuiltinNode?>>()
        )

    fun installBuiltin(builtin: NodeFactory<out LamaBuiltinNode?>) {
        EXTERNAL_BUILTINS.add(builtin)
    }

    override fun exitContext(context: LamaContext, exitMode: ExitMode?, exitCode: Int) {
        /*
         * Runs shutdown hooks during explicit exit triggered by TruffleContext#closeExit(Node, int)
         * or natural exit triggered during natural context close.
         */
        context.runShutdownHooks()
    }
}
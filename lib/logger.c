void logprintf(LOGGER log, unsigned level, char* fmt, ...){
	va_list args;

	va_start(args, fmt);
	if(log.verbosity>=level){
		vfprintf(log.stream, fmt, args);
	}
	va_end(args);
}
#define USE_DAZ_LOG 1

#include "ImageTools.h"

#if USE_DAZ_LOG
#include <dzapp.h>	
#endif

void log(QString message)
{
#if USE_DAZ_LOG
	dzApp->log(message);
#else
	printf(message.toLocal8Bit().constData());
#endif
}


// Bitwise check if number is a power of two
bool isPowerOfTwo(int n)
{
    // must be positive
    if (n <= 0) return false;
   
    // bitwise AND of n with n-1
    int bitwiseCalculation = n & (n-1);
    
    // if no other bits are set to 1, then n is power of 2
    if (bitwiseCalculation == 0) return true;

    // otherwise return false
    return false;
}

// Bitwise function to find the nearest power of two
// NOTE: will fail in edge case were n is equal or greater than the largest power of 2 that can be held by an int
int nearestPowerOfTwo(int n)
{
    // must be positive
    if (n < 1) return 0;

    // iterate through each bit position to generate a power of 2,
    // stop when power of 2 is greater than n
    // linear time dependent upon the number of bits in n (aka, fixed iterations limited to the bit-length of an int)
    int power = 1;
    while (power < n) {
        power <<= 1;
    }

    // calculate the previous power of 2
    int prevPower = power >> 1;
    
    // return the smallest difference
    return (power - n) < (n - prevPower) ? power : prevPower;
}

void multiplyImageByColorMultithreaded(QImage& image, QColor color)
{
	// Crash Check
	int width = image.width();
	int height = image.height();
	int lineLength = image.bytesPerLine();
	QImage::Format pixelFormat = image.format();
	if (pixelFormat != QImage::Format_ARGB32 &&
		pixelFormat != QImage::Format_RGB32)
	{
		log(QString("WARNING: multiplyImageByColorMultithreaded(): incompatible pixel format: %1, converting to ARGB32...").arg(pixelFormat));
		image = image.convertToFormat(QImage::Format_ARGB32);
	}

	int numThreads = QThreadPool::globalInstance()->maxThreadCount();
	int step = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int startY = i * step;
		int endY = (i == numThreads - 1) ? height : startY + step;
		MultiplyImageByColorTask* task = new MultiplyImageByColorTask(&image, color, startY, endY, width, height, pixelFormat);
		QThreadPool::globalInstance()->start(task);
	}

	QThreadPool::globalInstance()->waitForDone();
}

void multiplyImageByStrengthMultithreaded(QImage& image, double strength)
{
	// Crash Check
	int width = image.width();
	int height = image.height();
	int lineLength = image.bytesPerLine();
	QImage::Format pixelFormat = image.format();
	if (pixelFormat != QImage::Format_ARGB32 &&
		pixelFormat != QImage::Format_RGB32)
	{
		log(QString("WARNING: multiplyImageByStrengthMultithreaded(): incompatible pixel format: %1, converting to ARGB32...").arg(pixelFormat));
		image = image.convertToFormat(QImage::Format_ARGB32);
	}

	int numThreads = QThreadPool::globalInstance()->maxThreadCount();
	int step = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int startY = i * step;
		int endY = (i == numThreads - 1) ? height : startY + step;
		MultiplyImageByStrengthTask* task = new MultiplyImageByStrengthTask(&image, strength, startY, endY, width, height, pixelFormat);
		QThreadPool::globalInstance()->start(task);
	}

	QThreadPool::globalInstance()->waitForDone();
}

QString colorToHexString(const QColor& color)
{
	// Extract the RGB components
	int red = color.red();
	int green = color.green();
	int blue = color.blue();

	// Convert RGB to hexadecimal and concatenate
	QString hexColor = QString("#%1%2%3")
		.arg(QString::number(red, 16).rightJustified(2, '0'))
		.arg(QString::number(green, 16).rightJustified(2, '0'))
		.arg(QString::number(blue, 16).rightJustified(2, '0'));

	return hexColor.toUpper();
}

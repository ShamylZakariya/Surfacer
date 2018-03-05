//
//  Easing.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/3/18.
//

#ifndef Easing_hpp
#define Easing_hpp

#include "Common.hpp"
#include "MathHelpers.hpp"

namespace core {
    namespace util {
        namespace easing {
            
            namespace penner {
                
// penner functions make clang angry
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsequenced"
                
                /**
                 * Adapted from https://raw.githubusercontent.com/nobutaka/EasingCurvePresets/master/Assets/PennerDoubleAnimation.cs
                 * Changed from C# to C++ templated on type
                 */
                
                /**
                 * PennerDoubleAnimation
                 * Animates the value of a double property between two target values using
                 * Robert Penner's easing equations for interpolation over a specified Duration.
                 *
                 * @author        Darren David darren-code@lookorfeel.com
                 * @version        1.0
                 *
                 * Credit/Thanks:
                 * Robert Penner - The easing equations we all know and love
                 *   (http://robertpenner.com/easing/) [See License.txt for license info]
                 *
                 * Lee Brimelow - initial port of Penner's equations to WPF
                 *   (http://thewpfblog.com/?p=12)
                 *
                 * Zeh Fernando - additional equations (out/in) from
                 *   caurina.transitions.Tweener (http://code.google.com/p/tweener/)
                 *   [See License.txt for license info]
                 */
                
                /// <summary>
                /// Easing equation function for a simple linear tweening, with no easing.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T Linear(T t, T b, T c, T d)
                {
                    return c * t / d + b;
                }
                
                /// <summary>
                /// Easing equation function for an exponential (2^t) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T ExpoEaseOut(T t, T b, T c, T d)
                {
                    return ( t == d ) ? b + c : c * ( -pow( 2, -10 * t / d ) + 1 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for an exponential (2^t) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T ExpoEaseIn(T t, T b, T c, T d)
                {
                    return ( t == 0 ) ? b : c * pow( 2, 10 * ( t / d - 1 ) ) + b;
                }
                
                /// <summary>
                /// Easing equation function for an exponential (2^t) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T ExpoEaseInOut(T t, T b, T c, T d)
                {
                    if ( t == 0 )
                        return b;
                    
                    if ( t == d )
                        return b + c;
                    
                    if ( ( t /= d / 2 ) < 1 )
                        return c / 2 * pow( 2, 10 * ( t - 1 ) ) + b;
                    
                    return c / 2 * ( -pow( 2, -10 * --t ) + 2 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for an exponential (2^t) easing out/in:
                /// deceleration until halfway, then acceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T ExpoEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return ExpoEaseOut( t * 2, b, c / 2, d );
                    
                    return ExpoEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for a circular (sqrt(1-t^2)) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T CircEaseOut(T t, T b, T c, T d)
                {
                    return c * sqrt( 1 - ( t = t / d - 1 ) * t ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a circular (sqrt(1-t^2)) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T CircEaseIn(T t, T b, T c, T d)
                {
                    return -c * ( sqrt( 1 - ( t /= d ) * t ) - 1 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a circular (sqrt(1-t^2)) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T CircEaseInOut(T t, T b, T c, T d)
                {
                    if ( ( t /= d / 2 ) < 1 )
                        return -c / 2 * ( sqrt( 1 - t * t ) - 1 ) + b;
                    
                    return c / 2 * ( sqrt( 1 - ( t -= 2 ) * t ) + 1 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a circular (sqrt(1-t^2)) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T CircEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return CircEaseOut( t * 2, b, c / 2, d );
                    
                    return CircEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for a quadratic (t^2) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuadEaseOut(T t, T b, T c, T d)
                {
                    return -c * ( t /= d ) * ( t - 2 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a quadratic (t^2) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuadEaseIn(T t, T b, T c, T d)
                {
                    return c * ( t /= d ) * t + b;
                }
                
                /// <summary>
                /// Easing equation function for a quadratic (t^2) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuadEaseInOut(T t, T b, T c, T d)
                {
                    if ( ( t /= d / 2 ) < 1 )
                        return c / 2 * t * t + b;
                    
                    return -c / 2 * ( ( --t ) * ( t - 2 ) - 1 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a quadratic (t^2) easing out/in:
                /// deceleration until halfway, then acceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuadEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return QuadEaseOut( t * 2, b, c / 2, d );
                    
                    return QuadEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for a sinusoidal (sin(t)) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T SineEaseOut(T t, T b, T c, T d)
                {
                    return c * sin( t / d * ( M_PI / 2 ) ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a sinusoidal (sin(t)) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T SineEaseIn(T t, T b, T c, T d)
                {
                    return -c * cos( t / d * ( M_PI / 2 ) ) + c + b;
                }
                
                /// <summary>
                /// Easing equation function for a sinusoidal (sin(t)) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T SineEaseInOut(T t, T b, T c, T d)
                {
                    return -c/2 * (cos(M_PI*t/d) - 1) + b;
                }
                
                /// <summary>
                /// Easing equation function for a sinusoidal (sin(t)) easing in/out:
                /// deceleration until halfway, then acceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T SineEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return SineEaseOut( t * 2, b, c / 2, d );
                    
                    return SineEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for a cubic (t^3) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T CubicEaseOut(T t, T b, T c, T d)
                {
                    return c * ( ( t = t / d - 1 ) * t * t + 1 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a cubic (t^3) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T CubicEaseIn(T t, T b, T c, T d)
                {
                    return c * ( t /= d ) * t * t + b;
                }
                
                /// <summary>
                /// Easing equation function for a cubic (t^3) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T CubicEaseInOut(T t, T b, T c, T d)
                {
                    if ( ( t /= d / 2 ) < 1 )
                        return c / 2 * t * t * t + b;
                    
                    return c / 2 * ( ( t -= 2 ) * t * t + 2 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a cubic (t^3) easing out/in:
                /// deceleration until halfway, then acceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T CubicEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return CubicEaseOut( t * 2, b, c / 2, d );
                    
                    return CubicEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for a quartic (t^4) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuartEaseOut(T t, T b, T c, T d)
                {
                    return -c * ( ( t = t / d - 1 ) * t * t * t - 1 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a quartic (t^4) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuartEaseIn(T t, T b, T c, T d)
                {
                    return c * ( t /= d ) * t * t * t + b;
                }
                
                /// <summary>
                /// Easing equation function for a quartic (t^4) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuartEaseInOut(T t, T b, T c, T d)
                {
                    if ( ( t /= d / 2 ) < 1 )
                        return c / 2 * t * t * t * t + b;
                    
                    return -c / 2 * ( ( t -= 2 ) * t * t * t - 2 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a quartic (t^4) easing out/in:
                /// deceleration until halfway, then acceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuartEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return QuartEaseOut( t * 2, b, c / 2, d );
                    
                    return QuartEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for a quintic (t^5) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuintEaseOut(T t, T b, T c, T d)
                {
                    return c * ( ( t = t / d - 1 ) * t * t * t * t + 1 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a quintic (t^5) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuintEaseIn(T t, T b, T c, T d)
                {
                    return c * ( t /= d ) * t * t * t * t + b;
                }
                
                /// <summary>
                /// Easing equation function for a quintic (t^5) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuintEaseInOut(T t, T b, T c, T d)
                {
                    if ( ( t /= d / 2 ) < 1 )
                        return c / 2 * t * t * t * t * t + b;
                    return c / 2 * ( ( t -= 2 ) * t * t * t * t + 2 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a quintic (t^5) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T QuintEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return QuintEaseOut( t * 2, b, c / 2, d );
                    return QuintEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for an elastic (exponentially decaying sine wave) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T ElasticEaseOut(T t, T b, T c, T d)
                {
                    if ( ( t /= d ) == 1 )
                        return b + c;
                    
                    T p = d * .3;
                    T s = p / 4;
                    
                    return ( c * pow( 2, -10 * t ) * sin( ( t * d - s ) * ( 2 * M_PI ) / p ) + c + b );
                }
                
                /// <summary>
                /// Easing equation function for an elastic (exponentially decaying sine wave) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T ElasticEaseIn(T t, T b, T c, T d)
                {
                    if ( ( t /= d ) == 1 )
                        return b + c;
                    
                    T p = d * .3;
                    T s = p / 4;
                    
                    return -( c * pow( 2, 10 * ( t -= 1 ) ) * sin( ( t * d - s ) * ( 2 * M_PI ) / p ) ) + b;
                }
                
                /// <summary>
                /// Easing equation function for an elastic (exponentially decaying sine wave) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T ElasticEaseInOut(T t, T b, T c, T d)
                {
                    if ( ( t /= d / 2 ) == 2 )
                        return b + c;
                    
                    T p = d * ( .3 * 1.5 );
                    T s = p / 4;
                    
                    if ( t < 1 )
                        return -.5 * ( c * pow( 2, 10 * ( t -= 1 ) ) * sin( ( t * d - s ) * ( 2 * M_PI ) / p ) ) + b;
                    return c * pow( 2, -10 * ( t -= 1 ) ) * sin( ( t * d - s ) * ( 2 * M_PI ) / p ) * .5 + c + b;
                }
                
                /// <summary>
                /// Easing equation function for an elastic (exponentially decaying sine wave) easing out/in:
                /// deceleration until halfway, then acceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T ElasticEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return ElasticEaseOut( t * 2, b, c / 2, d );
                    return ElasticEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for a bounce (exponentially decaying parabolic bounce) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T BounceEaseOut(T t, T b, T c, T d)
                {
                    if ( ( t /= d ) < ( 1 / 2.75 ) )
                        return c * ( 7.5625 * t * t ) + b;
                    else if ( t < ( 2 / 2.75 ) )
                        return c * ( 7.5625 * ( t -= ( 1.5 / 2.75 ) ) * t + .75 ) + b;
                    else if ( t < ( 2.5 / 2.75 ) )
                        return c * ( 7.5625 * ( t -= ( 2.25 / 2.75 ) ) * t + .9375 ) + b;
                    else
                        return c * ( 7.5625 * ( t -= ( 2.625 / 2.75 ) ) * t + .984375 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a bounce (exponentially decaying parabolic bounce) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T BounceEaseIn(T t, T b, T c, T d)
                {
                    return c - BounceEaseOut<T>( d - t, 0, c, d ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a bounce (exponentially decaying parabolic bounce) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T BounceEaseInOut(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return BounceEaseIn<T>( t * 2, 0, c, d ) * .5 + b;
                    else
                        return BounceEaseOut<T>( t * 2 - d, 0, c, d ) * .5 + c * .5 + b;
                }
                
                /// <summary>
                /// Easing equation function for a bounce (exponentially decaying parabolic bounce) easing out/in:
                /// deceleration until halfway, then acceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T BounceEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return BounceEaseOut( t * 2, b, c / 2, d );
                    return BounceEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
                /// <summary>
                /// Easing equation function for a back (overshooting cubic easing: (s+1)*t^3 - s*t^2) easing out:
                /// decelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T BackEaseOut(T t, T b, T c, T d)
                {
                    return c * ( ( t = t / d - 1 ) * t * ( ( 1.70158 + 1 ) * t + 1.70158 ) + 1 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a back (overshooting cubic easing: (s+1)*t^3 - s*t^2) easing in:
                /// accelerating from zero velocity.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T BackEaseIn(T t, T b, T c, T d)
                {
                    return c * ( t /= d ) * t * ( ( 1.70158 + 1 ) * t - 1.70158 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a back (overshooting cubic easing: (s+1)*t^3 - s*t^2) easing in/out:
                /// acceleration until halfway, then deceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T BackEaseInOut(T t, T b, T c, T d)
                {
                    T s(1.70158);
                    if ( ( t /= d / 2 ) < 1 )
                        return c / 2 * ( t * t * ( ( ( s *= ( 1.525 ) ) + 1 ) * t - s ) ) + b;
                    return c / 2 * ( ( t -= 2 ) * t * ( ( ( s *= ( 1.525 ) ) + 1 ) * t + s ) + 2 ) + b;
                }
                
                /// <summary>
                /// Easing equation function for a back (overshooting cubic easing: (s+1)*t^3 - s*t^2) easing out/in:
                /// deceleration until halfway, then acceleration.
                /// </summary>
                /// <param name="t">Current time in seconds.</param>
                /// <param name="b">Starting value.</param>
                /// <param name="c">Final value.</param>
                /// <param name="d">Duration of animation.</param>
                /// <returns>The correct value.</returns>
                template<typename T> T BackEaseOutIn(T t, T b, T c, T d)
                {
                    if ( t < d / 2 )
                        return BackEaseOut( t * 2, b, c / 2, d );
                    return BackEaseIn( ( t * 2 ) - d, b + c / 2, c / 2, d );
                }
                
#pragma clang diagnostic pop
                
            } // end namespace penner
            
#pragma mark - Linear

            template<typename T> T linear(T t)
            {
                return penner::Linear<T>(t, 0, 1, 1);
            }
            
#pragma mark - Expo
            
            template<typename T> T expo_ease_out(T t)
            {
                return penner::ExpoEaseOut<T>(t, 0, 1, 1);
            }
            
            template<typename T> T expo_ease_in(T t)
            {
                return penner::ExpoEaseIn<T>(t, 0, 1, 1);
            }
            
            template<typename T> T expo_ease_in_out(T t)
            {
                return penner::ExpoEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T expo_ease_out_in(T t)
            {
                return penner::ExpoEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Circ

            template<typename T> T circ_ease_out(T t)
            {
                return penner::CircEaseOut<T>(t,0,1,1);
            }

            template<typename T> T circ_ease_in(T t)
            {
                return penner::CircEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T circ_ease_in_out(T t)
            {
                return penner::CircEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T circ_ease_out_in(T t)
            {
                return penner::CircEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Quad
            
            template<typename T> T quad_ease_out(T t)
            {
                return penner::QuadEaseOut<T>(t,0,1,1);
            }
            
            template<typename T> T quad_ease_in(T t)
            {
                return penner::QuadEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T quad_ease_in_out(T t)
            {
                return penner::QuadEaseInOut<T>(t,0,1,1);
            }

            template<typename T> T quad_ease_out_in(T t)
            {
                return penner::QuadEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Sine
            
            template<typename T> T sine_ease_out(T t)
            {
                return penner::SineEaseOut<T>(t,0,1,1);
            }
            
            template<typename T> T sine_ease_in(T t)
            {
                return penner::SineEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T sine_ease_in_out(T t)
            {
                return penner::SineEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T sine_ease_out_in(T t)
            {
                return penner::SineEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Cubic
            
            template<typename T> T cubic_ease_out(T t)
            {
                return penner::CubicEaseOut<T>(t,0,1,1);
            }
            
            template<typename T> T cubic_ease_in(T t)
            {
                return penner::CubicEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T cubic_ease_in_out(T t)
            {
                return penner::CubicEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T cubic_ease_out_in(T t)
            {
                return penner::CubicEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Quartic
            
            template<typename T> T quart_ease_out(T t)
            {
                return penner::QuartEaseOut<T>(t,0,1,1);
            }
            
            template<typename T> T quart_ease_in(T t)
            {
                return penner::QuartEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T quart_ease_in_out(T t)
            {
                return penner::QuartEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T quart_ease_out_in(T t)
            {
                return penner::QuartEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Quintic
            
            template<typename T> T quint_ease_out(T t)
            {
                return penner::QuintEaseOut<T>(t,0,1,1);
            }
            
            template<typename T> T quint_ease_in(T t)
            {
                return penner::QuintEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T quint_ease_in_out(T t)
            {
                return penner::QuintEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T quint_ease_out_in(T t)
            {
                return penner::QuintEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Elastic
            
            template<typename T> T elastic_ease_out(T t)
            {
                return penner::ElasticEaseOut<T>(t,0,1,1);
            }
            
            template<typename T> T elastic_ease_in(T t)
            {
                return penner::ElasticEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T elastic_ease_in_out(T t)
            {
                return penner::ElasticEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T elastic_ease_out_in(T t)
            {
                return penner::ElasticEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Bounce
            
            template<typename T> T bounce_ease_out(T t)
            {
                return penner::BounceEaseOut<T>(t,0,1,1);
            }
            
            template<typename T> T bounce_ease_in(T t)
            {
                return penner::BounceEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T bounce_ease_in_out(T t)
            {
                return penner::BounceEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T bounce_ease_out_in(T t)
            {
                return penner::BounceEaseOutIn<T>(t,0,1,1);
            }
            
#pragma mark - Back
            
            template<typename T> T back_ease_out(T t)
            {
                return penner::BackEaseOut<T>(t,0,1,1);
            }
            
            template<typename T> T back_ease_in(T t)
            {
                return penner::BackEaseIn<T>(t,0,1,1);
            }
            
            template<typename T> T back_ease_in_out(T t)
            {
                return penner::BackEaseInOut<T>(t,0,1,1);
            }
            
            template<typename T> T back_ease_out_in(T t)
            {
                return penner::BackEaseOutIn<T>(t,0,1,1);
            }
            
        } // end namespace easing
    } // end namespace util
} // end namespace core


#endif /* Easing_hpp */

